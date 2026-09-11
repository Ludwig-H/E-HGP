// Successor accounting v2: an independent operation-prefix model judges the
// REAL helper, then real Gabriel catalogues exercise the FULL constructor.
// Explicit helper reuse: full_gabriel_singleton_fixtures.hpp at SHA256
// 600701d340b88ad66efda7ade6a3c403bc1db4c194c8a33fd5a4a5a789432f84.
// Its exhaustive Gamma oracle full_gamma.hpp is independently pinned at
// a17732d2bd7861a3e7e3f76d029da3b2078ce4ebf0b64f7d7571e5060de24f0c.
// The private old compression branch is only a differential, not an oracle.
#include <cstring>
#include <exception>

#include "full_gabriel_singleton_fixtures.hpp"

#if !defined(MHGP7_TESTING)
#error "successor qualification requires the private legacy differential"
#endif

using namespace mhgp7_singleton_test;
namespace {
namespace detail = full_gabriel_detail;
using Status = detail::SuccessorStatus;
u64 primitive_cases = 0, primitive_calls = 0, primitive_successes = 0, primitive_refusals = 0;
u64 search_refusals = 0, compression_refusals = 0, before_write_refusals = 0;
u64 depth_one_successes = 0, unknown_cases = 0, repeated_sequences = 0, cumulative_refusals = 0;
u64 depth_cases[6]{};
u64 full_pairs = 0, full_calls = 0, full_refusals = 0, full_positive_pairs = 0;
u64 order_cases = 0, permutations = 0, terminal_orders = 0, positive_depth_orders = 0;
u64 exact_caps = 0, short_caps = 0, successor_exact_caps = 0, successor_short_caps = 0;
u64 distinct_admissions = 0, saved_steps = 0;

struct LegacyScope {
  bool prior = detail::force_legacy_successor;
  explicit LegacyScope(bool legacy) { detail::force_legacy_successor = legacy; }
  ~LegacyScope() { detail::force_legacy_successor = prior; }
  LegacyScope(const LegacyScope&) = delete;
  LegacyScope& operator=(const LegacyScope&) = delete;
};

struct PrimitiveState {
  std::vector<FullNodeId> successor;
  FullNodeId root = std::numeric_limits<FullNodeId>::max() - 1;
  u64 steps = 0, anchors = 0;
};
struct Model {
  PrimitiveState state;
  Status status = Status::kUnknownAnchor;
  u64 depth = 0, operations = 0;
};

// Decode the immutable graph once. The expected mutations below are selected
// directly from the contract's operation trace, not by another compression
// loop. Off-path nodes remain in the copied state and are compared literally.
Model model(const PrimitiveState& before, FullNodeId token, u64 cap, bool legacy) {
  Model out{before, Status::kUnknownAnchor, 0, 0};
  if (token >= before.successor.size()) return out;
  std::vector<FullNodeId> path;
  FullNodeId at = token;
  while (path.size() <= before.successor.size()) {
    if (at >= before.successor.size()) { check(false, "model graph edge in bounds"); return out; }
    path.push_back(at);
    const FullNodeId next = before.successor[static_cast<size_t>(at)];
    if (next == at) break;
    at = next;
  }
  check(!path.empty() && path.size() <= before.successor.size() &&
        before.successor[static_cast<size_t>(path.back())] == path.back(), "model graph is a rooted forest");
  if (path.empty() || path.size() > before.successor.size()) return out;
  out.depth = path.size() - 1;
  out.operations = legacy ? 3 * out.depth + 1 : out.depth == 0 ? 1 : 3 * out.depth - 1;
  const u64 available = cap > before.steps ? cap - before.steps : 0;
  const u64 admitted = std::min(available, out.operations);
  out.state.steps += admitted;
  out.state.root = path[static_cast<size_t>(std::min(admitted, out.depth))];
  const u64 search = out.depth + 1;
  if (out.depth > 0 && admitted >= search) ++out.state.anchors;
  const u64 pairs = admitted > search ? (admitted - search) / 2 : 0;
  const u64 writes = std::min(pairs, legacy ? out.depth : out.depth == 0 ? 0 : out.depth - 1);
  for (u64 i = 0; i < writes; ++i)
    out.state.successor[static_cast<size_t>(path[static_cast<size_t>(i)])] = path.back();
  out.status = admitted == out.operations ? Status::kOk : Status::kBudget;
  return out;
}

Status exercise(PrimitiveState& state, FullNodeId token, u64 cap, bool legacy) {
  const Model wanted = model(state, token, cap, legacy);
  const u64 initial_steps = state.steps;
  Status status;
  {
    LegacyScope scope(legacy);
    status = detail::normalize_successor(state.successor, token, state.root,
                                        state.steps, state.anchors, cap);
  }
  ++primitive_calls;
  check(!detail::force_legacy_successor, "primitive differential switch restored");
  check(status == wanted.status, "primitive exact success/unknown/budget status");
  check(state.successor == wanted.state.successor, "ENTIRE successor array equals independent operation prefix");
  check(state.root == wanted.state.root, "root output equals admitted search prefix, also on refusals");
  check(state.steps == wanted.state.steps, "prospective charged operations, without refund or budget reset");
  check(state.anchors == wanted.state.anchors, "normalized_anchors increments after the terminal read, even before a compression refusal");
  if (!legacy) {
    if (status == Status::kOk) {
      ++primitive_successes;
      if (wanted.depth == 1) ++depth_one_successes;
    } else if (status == Status::kBudget) {
      ++primitive_refusals;
      const u64 admitted = state.steps - initial_steps;
      if (admitted < wanted.depth + 1) ++search_refusals;
      else {
        ++compression_refusals;
        if ((admitted - wanted.depth - 1) % 2 == 1) ++before_write_refusals;
      }
    }
  }
  return status;
}

PrimitiveState chain(unsigned depth, bool spaced, u64 initial_steps) {
  PrimitiveState out;
  const size_t stride = spaced ? 2 : 1;
  out.successor.resize(stride * depth + 4);
  std::iota(out.successor.begin(), out.successor.end(), FullNodeId{0});
  for (unsigned i = 0; i < depth; ++i) out.successor[1 + stride * i] = 1 + stride * (i + 1);
  if (spaced) out.successor[0] = 1 + stride * (depth / 2);
  out.successor[out.successor.size() - 2] = out.successor.size() - 1;
  out.steps = initial_steps;
  out.anchors = initial_steps == 0 ? 0 : 5;
  return out;
}

void primitives() {
  const unsigned depths[] = {0, 1, 2, 3, 7, 31};
  for (size_t di = 0; di < 6; ++di) {
    const unsigned depth = depths[di];
    const u64 cost = depth == 0 ? 1 : 3 * depth - 1;
    for (bool spaced : {false, true}) for (u64 initial : {u64{0}, u64{17}}) {
      context = "primitive/d=" + std::to_string(depth) + "/spaced=" + std::to_string(spaced) +
                "/initial=" + std::to_string(initial);
      for (u64 allowance = 0; allowance <= cost + 1; ++allowance) {
        ++primitive_cases; ++depth_cases[di];
        auto fast = chain(depth, spaced, initial), old = fast;
        const auto status = exercise(fast, 1, initial + allowance, false);
        exercise(old, 1, initial + allowance, true);
        if (status == Status::kOk) {
          auto complete_old = chain(depth, spaced, initial);
          check(exercise(complete_old, 1, initial + 3 * depth + 1, true) == Status::kOk,
                "old successful-state sentinel admitted");
          check(fast.successor == complete_old.successor && fast.root == complete_old.root &&
                fast.anchors == complete_old.anchors, "v2 final array, root and anchors equal old full compression");
        }
      }
    }
  }
  // Unknown anchors win even when the numerical budget is already exhausted.
  for (u64 initial : {u64{0}, u64{17}}) for (bool legacy : {false, true})
    for (bool sentinel : {false, true}) {
      context = "primitive/unknown";
      auto s = chain(2, true, initial);
      const FullNodeId token = sentinel ? std::numeric_limits<FullNodeId>::max() : s.successor.size();
      check(exercise(s, token, initial, legacy) == Status::kUnknownAnchor, "unknown anchor rejected before any charge");
      ++unknown_cases;
    }
  for (bool legacy : {false, true}) {
    context = "primitive/cap_below_spent";
    auto s = chain(3, true, 17);
    check(exercise(s, 1, 16, legacy) == Status::kBudget && s.steps == 17,
          "existing cumulative consumption is never reduced to the cap");
  }
  for (unsigned depth : {3u, 31u}) for (bool spaced : {false, true}) {
    context = "primitive/repeated/d=" + std::to_string(depth) + "/spaced=" + std::to_string(spaced);
    auto fast = chain(depth, spaced, 17), old = fast;
    const FullNodeId root = 1 + (spaced ? 2 : 1) * depth;
    const FullNodeId branch = spaced ? 0 : fast.successor.size() - 2;
    const FullNodeId middle = 1 + (spaced ? 2 : 1) * (depth / 2);
    for (FullNodeId token : {FullNodeId{1}, FullNodeId{1}, root, branch, middle, root}) {
      check(exercise(fast, token, 10000, false) == Status::kOk &&
            exercise(old, token, 10000, true) == Status::kOk, "repeated calls admitted cumulatively");
      check(fast.successor == old.successor && fast.root == old.root && fast.anchors == old.anchors,
            "induction on repeated calls: entire successor state remains equal");
      check(old.steps >= fast.steps && old.steps - fast.steps == 2 * (fast.anchors - 5),
            "successful cumulative v2 savings equal two per positive-depth call");
    }
    ++repeated_sequences;
    // This is a fresh sequence, not a product retry after a global refusal.
    for (u64 leftover : {u64{0}, u64{1}}) {
      auto s = chain(depth, spaced, 17);
      const u64 cap = 17 + 3 * depth - 1 + leftover;
      check(exercise(s, 1, cap, false) == Status::kOk, "first call fits the cumulative cap");
      check(exercise(s, 1, cap, false) == Status::kBudget, "second depth-one call consumes the remaining prefix without reset");
      ++cumulative_refusals;
    }
  }
}

u64 cache_size(int policy) { return policy < 2 ? 0 : policy == 2 ? 1 : 1000000; }
FullGabrielResult full(const CloudIndex& ix, unsigned k, const std::vector<ForestEvent>& minima,
                       const std::vector<ForestEvent>& direct, int policy,
                       const FullGabrielLimits& caps, bool legacy) {
  FullGabrielResult out;
  {
    LegacyScope scope(legacy);
    out = policy == 0 ? build_full_gabriel_order(ix, k, minima, direct, caps) :
                       build_full_gabriel_order_lazy(ix, k, minima, direct, caps, {cache_size(policy)});
  }
  ++full_calls;
  check(!detail::force_legacy_successor, "FULL differential switch restored");
  check(out.successor_accounting && std::strcmp(out.successor_accounting,
        legacy ? "full_successor_reads_writes_v1" : "full_successor_reads_writes_no_last_pair_v2") == 0,
        "explicit v1/v2 accounting label, including refused results");
  check(out.alias_policy && std::strcmp(out.alias_policy,
        policy == 0 ? kFullGabrielEagerAliases : kFullGabrielLazyAliases) == 0, "alias policy unchanged");
  if (out.status != FullGabrielStatus::kCompleteRelative) {
    ++full_refusals;
    check(empty(out.forest), "refusal invalidates order, nodes, minima and parent arenas");
  }
  return out;
}
FullGabrielResult full(const Cloud& c, unsigned k, int policy, const FullGabrielLimits& caps, bool legacy) {
  return full(c.ix, k, c.catalogue[k], c.catalogue[k+1], policy, caps, legacy);
}
void compare(const FullGabrielResult& fast, const FullGabrielResult& old) {
  ++full_pairs;
  check(fast.status == old.status && fast.reason && old.reason && std::strcmp(fast.reason, old.reason) == 0,
        "same status and reason when successor limits do not interrupt comparison");
  check(same_forest(fast.forest, old.forest), "literal FULL forest equals retained old compression");
  auto other = fast.stats;
  other.successor_steps = old.stats.successor_steps;
  check(work(other) == work(old.stats), "ALL 32 other FULL/geometry fields agree");
  if (fast.status == FullGabrielStatus::kCompleteRelative && old.status == fast.status) {
    ++full_positive_pairs;
    check(fast.reason && std::strcmp(fast.reason, kFullGabrielAuthority) == 0, "success remains relative to supplied complete exact regular catalogues");
    check(old.stats.successor_steps >= 2 * old.stats.normalized_anchors &&
          fast.stats.successor_steps == old.stats.successor_steps - 2 * old.stats.normalized_anchors,
          "SUCCESS ONLY: S_v2 = S_v1 - 2A");
  }
  // No success identity is applied to either interrupted successor walk.
}
void refusal(const FullGabrielResult& out, const char* reason) {
  check(out.status == FullGabrielStatus::kResourceExhausted && out.reason &&
        std::strcmp(out.reason, reason) == 0 && empty(out.forest), reason);
}

void qualify(const Cloud& c, bool permute) {
  if (!c.valid) return;
  for (unsigned k = 1; k <= c.in.size(); ++k) {
    ++order_cases;
    for (int policy = 0; policy < 4; ++policy) {
      context = c.name + "/K=" + std::to_string(k) + "/policy=" + std::to_string(policy);
      const auto caps = roomy(policy != 0);
      const auto fast = full(c, k, policy, caps, false), old = full(c, k, policy, caps, true);
      compare(fast, old);
      if (fast.status != FullGabrielStatus::kCompleteRelative) { check(false, "nominal FULL call succeeds"); continue; }
      compare_oracle(c, k, fast.forest);
      if (fast.stats.normalized_anchors > 0) ++positive_depth_orders;
      saved_steps += old.stats.successor_steps - fast.stats.successor_steps;
      if (k == c.in.size()) {
        ++terminal_orders;
        check(fast.forest.nodes().size() == 1 && fast.forest.minima().size() == 1 &&
              fast.forest.parents().empty(), "K=n retains the terminal minimum");
      }
      if (permute && k == 2) {
        auto points = c.in; std::reverse(points.begin(), points.end());
        auto minima = c.catalogue[k], direct = c.catalogue[k+1];
        std::reverse(minima.begin(), minima.end()); std::reverse(direct.begin(), direct.end());
        const auto ix = build_cloud_index(points);
        const auto changed = full(ix, k, minima, direct, policy, caps, false);
        const auto old_changed = full(ix, k, minima, direct, policy, caps, true);
        compare(changed, old_changed);
        check(changed.status == FullGabrielStatus::kCompleteRelative && same_forest(fast.forest, changed.forest) &&
              work(fast.stats) == work(changed.stats), "permutations preserve v2 forest and all 33 v2 counters");
        ++permutations;
      }
    }
  }
}

void budgets(const Cloud& c, unsigned k) {
  if (!c.valid) return;
  for (int policy = 0; policy < 4; ++policy) {
    context = c.name + "/budgets/K=" + std::to_string(k) + "/policy=" + std::to_string(policy);
    const auto base_caps = roomy(policy != 0);
    const auto fast = full(c, k, policy, base_caps, false), old = full(c, k, policy, base_caps, true);
    compare(fast, old);
    if (fast.status != FullGabrielStatus::kCompleteRelative || old.status != fast.status) {
      check(false, "budget sentinel succeeds"); continue;
    }
    struct Limit { u64 FullGabrielLimits::*field; u64 used; const char* reason; };
    const Limit fields[] = {
      {&FullGabrielLimits::max_points, c.in.size(), "full_gabriel_point_budget"},
      {&FullGabrielLimits::max_input_records, fast.stats.input_records, "full_gabriel_input_budget"},
      {&FullGabrielLimits::max_face_visits, fast.stats.face_visits, "full_gabriel_face_budget"},
      {&FullGabrielLimits::max_aliases, fast.stats.aliases, "full_gabriel_alias_budget"},
      {&FullGabrielLimits::max_portal_requests, fast.stats.portal_requests, "full_gabriel_portal_budget"},
      {&FullGabrielLimits::max_chain_steps, fast.stats.chain_steps, "full_gabriel_chain_budget"},
      {&FullGabrielLimits::max_meb_calls, fast.stats.meb_calls, "full_gabriel_meb_call_budget"},
      {&FullGabrielLimits::max_query_nodes, fast.stats.geometry.query_nodes, "silent_query_node_budget"},
      {&FullGabrielLimits::max_meb_supports, fast.stats.geometry.meb_supports, "silent_meb_support_budget"}};
    for (const auto& field : fields) {
      auto caps = base_caps; caps.*field.field = field.used;
      const auto a = full(c, k, policy, caps, false), b = full(c, k, policy, caps, true);
      compare(a, b);
      check(a.status == FullGabrielStatus::kCompleteRelative && same_forest(a.forest, fast.forest) &&
            work(a.stats) == work(fast.stats), "exact non-successor cap preserves the v2 sentinel");
      ++exact_caps;
      if (field.used == 0) continue;
      for (u64 cap : {u64{0}, field.used - 1}) {
        caps.*field.field = cap;
        const auto ar = full(c, k, policy, caps, false), br = full(c, k, policy, caps, true);
        compare(ar, br); refusal(ar, field.reason); refusal(br, field.reason); ++short_caps;
      }
    }
    u64 batches = 0;
    for (size_t i = 0; i < fast.forest.nodes().size(); ++i)
      if (i == 0 || !same_exact_level(fast.forest.nodes()[i-1].level, fast.forest.nodes()[i].level)) ++batches;
    struct CertLimit { u64 FullCertificateLimits::*field; u64 used; const char* reason; };
    const CertLimit cert[] = {
      {&FullCertificateLimits::max_batches, batches, "full_gabriel_batch_budget"},
      {&FullCertificateLimits::max_nodes, fast.forest.nodes().size(), "full_gabriel_node_budget"},
      {&FullCertificateLimits::max_parent_refs, fast.forest.parents().size(), "full_gabriel_parent_budget"}};
    for (const auto& field : cert) {
      auto caps = base_caps; caps.certificate.*field.field = field.used;
      const auto a = full(c, k, policy, caps, false), b = full(c, k, policy, caps, true);
      compare(a, b);
      check(a.status == FullGabrielStatus::kCompleteRelative && same_forest(a.forest, fast.forest) &&
            work(a.stats) == work(fast.stats), "exact certificate cap preserves the v2 sentinel");
      ++exact_caps;
      if (field.used == 0) continue;
      for (u64 cap : {u64{0}, field.used - 1}) {
        caps.certificate.*field.field = cap;
        const auto ar = full(c, k, policy, caps, false), br = full(c, k, policy, caps, true);
        compare(ar, br); refusal(ar, field.reason); refusal(br, field.reason); ++short_caps;
      }
    }
    // Same numerical successor cap is deliberately NOT a same-admission
    // differential. Each calendar is first qualified at its own exact total.
    for (bool legacy : {false, true}) {
      const auto& baseline = legacy ? old : fast;
      auto caps = base_caps; caps.max_successor_steps = baseline.stats.successor_steps;
      const auto exact = full(c, k, policy, caps, legacy);
      check(exact.status == FullGabrielStatus::kCompleteRelative && same_forest(exact.forest, baseline.forest) &&
            work(exact.stats) == work(baseline.stats), "own-calendar exact successor cap preserves all work");
      ++successor_exact_caps;
      if (baseline.stats.successor_steps > 0) {
        --caps.max_successor_steps;
        refusal(full(c, k, policy, caps, legacy), "full_gabriel_successor_budget"); ++successor_short_caps;
        caps.max_successor_steps = 0;
        refusal(full(c, k, policy, caps, legacy), "full_gabriel_successor_budget"); ++successor_short_caps;
      }
    }
    if (fast.stats.normalized_anchors > 0) {
      auto caps = base_caps; caps.max_successor_steps = fast.stats.successor_steps;
      const auto admitted = full(c, k, policy, caps, false), denied = full(c, k, policy, caps, true);
      check(admitted.status == FullGabrielStatus::kCompleteRelative && same_forest(admitted.forest, fast.forest),
            "v2 exact cap admits its complete sentinel");
      refusal(denied, "full_gabriel_successor_budget");
      ++distinct_admissions;
    }
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  const bool rejects = std::strcmp(argv[1], "--rejects") == 0;
  mhgp7_oracle::clear_overflow();
  try {
    check(std::strcmp(kFullGabrielSuccessorAccounting, "full_successor_reads_writes_no_last_pair_v2") == 0,
          "published successor-accounting identifier is exactly v2");
    primitives();
    Cloud single("single", input({{7,11,13}}, true)); qualify(single, false);
    Cloud acute("acute", input({{0,0,0},{6,0,0},{2,3,0}})); qualify(acute, false);
    Cloud obtuse("obtuse", input({{0,0,0},{6,0,0},{1,1,0}})); qualify(obtuse, false);
    Cloud tetra("tetra", input({{20,20,20},{24,24,20},{24,20,24},{20,24,24}})); qualify(tetra, true);
    Cloud mixed("mixed", input({{25,20,20},{17,24,20},{17,16,20},{120,20,20},{130,20,20}})); qualify(mixed, true);
    const std::vector<P3> e5{{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}};
    Cloud original("E5", input(e5)); qualify(original, true);
    Cloud sparse("E5_sparse_s10", input(e5, true), 10); qualify(sparse, true);
    const std::vector<P3> points{{622,745,858},{839,341,867},{111,242,715},{827,10,537},
                               {437,578,984},{396,213,30},{693,305,961},{814,71,415}};
    Cloud two_step("two_step_s12", input(points), 12); qualify(two_step, true);
    Cloud shared("J1_shared_lot", input({{0,50,0},{40,50,0},{20,61,0},{20,0,0},{20,10,30}})); qualify(shared, true);
    if (rejects) { budgets(original, 2); budgets(two_step, 2); budgets(tetra, 3); budgets(mixed, 2); }
  } catch (const std::exception& e) {
    ++failures; std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), e.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "sticky independent OBig overflow remains clear");
  check(!mhgp7::full_gabriel_detail::force_legacy_successor, "legacy switch remains restored at exit");
  const bool floor = primitive_cases == 560 && primitive_successes > 0 && primitive_refusals > 0 &&
      search_refusals > 0 && compression_refusals > 0 && before_write_refusals > 0 &&
      depth_one_successes > 0 && unknown_cases == 8 && repeated_sequences == 4 && cumulative_refusals == 8 &&
      std::all_of(std::begin(depth_cases), std::end(depth_cases), [](u64 n) { return n > 0; }) &&
      admitted_clouds == 9 && order_cases == 39 && permutations == 24 && terminal_orders == 36 &&
      positive_depth_orders > 0 && saved_steps > 0 && cuts >= 3320 && records >= 148 &&
      (!rejects || (exact_caps == 192 && short_caps >= 200 && successor_exact_caps == 32 &&
                    successor_short_caps == 64 && distinct_admissions == 16));
  std::printf("full_gabriel_successor mode=%s primitive_cases=%llu primitive_calls=%llu primitive_success=%llu "
              "primitive_refused=%llu search_refused=%llu compression_refused=%llu before_write_refused=%llu "
              "d0=%llu d1=%llu d2=%llu d3=%llu d7=%llu d31=%llu depth1_success=%llu unknown=%llu "
              "repeated_sequences=%llu cumulative_refused=%llu clouds=%llu orders=%llu full_pairs=%llu "
              "full_calls=%llu full_positive_pairs=%llu full_refused=%llu cuts=%llu records=%llu "
              "permutations=%llu terminals=%llu positive_depth_orders=%llu saved_steps=%llu "
              "exact_caps=%llu short_caps=%llu successor_exact=%llu successor_short=%llu distinct_admissions=%llu "
              "checks=%llu failures=%llu floor=%d\n", argv[1],
      (unsigned long long)primitive_cases, (unsigned long long)primitive_calls, (unsigned long long)primitive_successes,
      (unsigned long long)primitive_refusals, (unsigned long long)search_refusals, (unsigned long long)compression_refusals,
      (unsigned long long)before_write_refusals, (unsigned long long)depth_cases[0], (unsigned long long)depth_cases[1],
      (unsigned long long)depth_cases[2], (unsigned long long)depth_cases[3], (unsigned long long)depth_cases[4],
      (unsigned long long)depth_cases[5], (unsigned long long)depth_one_successes, (unsigned long long)unknown_cases,
      (unsigned long long)repeated_sequences, (unsigned long long)cumulative_refusals, (unsigned long long)admitted_clouds,
      (unsigned long long)order_cases, (unsigned long long)full_pairs, (unsigned long long)full_calls,
      (unsigned long long)full_positive_pairs, (unsigned long long)full_refusals, (unsigned long long)cuts,
      (unsigned long long)records, (unsigned long long)permutations, (unsigned long long)terminal_orders,
      (unsigned long long)positive_depth_orders, (unsigned long long)saved_steps, (unsigned long long)exact_caps,
      (unsigned long long)short_caps, (unsigned long long)successor_exact_caps, (unsigned long long)successor_short_caps,
      (unsigned long long)distinct_admissions, (unsigned long long)checks, (unsigned long long)failures, (int)floor);
  return failures ? 1 : floor ? 0 : 3;
}
