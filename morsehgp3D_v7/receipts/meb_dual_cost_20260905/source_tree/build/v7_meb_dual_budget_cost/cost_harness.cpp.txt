// Private preparation: never compiled or timed by this change.
// The runner must pin the final geometry donor, its completed receipt, every
// include, this file and the measured binary. This is not a standalone receipt.
// Explicit inert reuse: no call to the renamed donor main/run or its mutant.
#define main mhgp7_geometry_gate_uninvoked_main
#include "../v7_meb_dual_budget_geometry/geometry_gate.cpp"
#undef main

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>

#if defined(MHGP7_TESTING) || defined(__SANITIZE_ADDRESS__)
#error "Cost nominal must not be a testing/sanitized translation unit"
#endif

namespace {
namespace cost {

constexpr u64 kMaxCalls = 2000000;
constexpr size_t kWarmups = 2, kPasses = 7, kPairRepeats = 4096;
constexpr u64 kHashSeed = 1469598103934665603ULL;
using Clock = std::chrono::steady_clock;
static_assert(Clock::is_steady);

// Count both top-level entries and actual nested F fallback entries. The
// maximum is reserved before a batch; no delayed charge permits extra work.
struct CallLedger {
  u64 entries = 0;
  void reserve(u64 maximum) const {
    check(maximum <= kMaxCalls && entries <= kMaxCalls - maximum, "cost.call_limit");
  }
  void add(u64 actual) { reserve(actual); entries += actual; }
};

void mix(u64* hash, u64 word) {
  *hash = (*hash ^ word) * 1099511628211ULL;
}

void mix128(u64* hash, i128 word) {
  const u128 bits = static_cast<u128>(word);
  mix(hash, static_cast<u64>(bits));
  mix(hash, static_cast<u64>(bits >> 64));
}

void mix_level(u64* hash, const ExactLevel& level) {
  for (const auto word : level.num) mix(hash, word);
  mix128(hash, level.den);
}

// Named fields, not padding/object addresses. Full comparisons below remain
// authoritative: this rolling capture is observable work, not a new oracle.
u64 terminal_hash(bool ok, const SilentIncidenceResult& out,
                  const silent_detail::LocalBall& ball) {
  u64 hash = kHashSeed;
  mix(&hash, ok); mix(&hash, static_cast<u64>(out.status));
  for (const char* text = out.reason; *text; ++text) mix(&hash, static_cast<unsigned char>(*text));
  mix(&hash, 0);
  const auto& s = out.stats;
  for (u64 word : {s.core_records,s.core_facets,s.facets_with_two_intruders,s.chain_steps,
      s.added_cofaces,s.terminal_direct,s.terminal_cached,s.max_chain_length,s.query_nodes,
      s.query_leaves,s.query_range_skips,s.meb_calls,s.meb_supports}) mix(&hash, word);
  mix128(&hash, ball.key.a);
  for (const auto word : ball.key.b) mix128(&hash, word);
  mix128(&hash, ball.key.c); mix_level(&hash, ball.level); mix(&hash, ball.q);
  for (const auto site : ball.support) mix(&hash, static_cast<u64>(site));
  mix(&hash, out.events.size());
  for (const auto& event : out.events) {
    mix(&hash, event.q); mix(&hash, event.d); mix(&hash, event.active_mask);
    mix_level(&hash, event.level);
    for (const auto id : event.support) mix(&hash, id);
    for (const auto id : event.interior) mix(&hash, id);
  }
  return hash;
}

u64 work_hash(const proposal::Work& work) {
  u64 hash = kHashSeed;
  for (const u64 word : {work.meb_proposal_supports,work.pivots,work.certified,work.fallback})
    mix(&hash, word);
  return hash;
}

bool same_work(const proposal::Work& a, const proposal::Work& b) {
  return a.meb_proposal_supports == b.meb_proposal_supports && a.pivots == b.pivots &&
      a.certified == b.certified && a.fallback == b.fallback;
}

struct ArmState {
  SilentIncidenceResult out;
  proposal::Work work;
};

struct Outcome {
  bool ok = false;
  silent_detail::LocalBall ball;
};

struct Job {
  size_t id = 0, fixture_id = 0, order_id = 0;
  Order order{};
  u64 rank = 0, legacy_cap = 0, proposal_cap = 0, legacy_start = 0, proposal_start = 0;
  size_t pivot_cap = 16, steps = 1, repeats = 1;
  const char* cohort = "main";
  SilentIncidenceResult seed;
  std::vector<u64> expected_terminal, expected_work;
  std::string route, terminals;
  u8 reference_q = 0, first_result_q = 0;
};

// GCC noipa closes interprocedural specialization/cloning as well as inlining.
// No LTO and emitted disassembly must also be checked before the measurement GO.
using Invoke = bool (*)(const Fixture&, const Job&, ArmState*, Outcome*);

__attribute__((noinline, noipa))
bool invoke_f(const Fixture& f, const Job& job, ArmState* state, Outcome* result) {
  asm volatile("" : : "g"(&f), "g"(&job), "g"(state), "g"(result) : "memory");
  SilentIncidenceLimits caps;
  caps.max_meb_supports = job.legacy_cap;
  const std::vector<ForestEvent> direct;
  silent_detail::Builder builder(f.ix, direct, caps, &state->out);
  result->ok = builder.miniball(job.order.sites, f.n, &result->ball);
  asm volatile("" : : "m"(*result), "m"(*state) : "memory");
  return result->ok;
}

__attribute__((noinline, noipa))
bool invoke_dual(const Fixture& f, const Job& job, ArmState* state, Outcome* result) {
  asm volatile("" : : "g"(&f), "g"(&job), "g"(state), "g"(result) : "memory");
  SilentIncidenceLimits caps;
  caps.max_meb_supports = job.legacy_cap;
  const std::vector<ForestEvent> direct;
  proposal::NoObserver observer;
  result->ok = proposal::miniball<false>(f.ix, direct, caps, &state->out,
      job.order.sites, f.n, &result->ball, proposal::Limits{job.proposal_cap},
      &state->work, &observer, job.pivot_cap);
  asm volatile("" : : "m"(*result), "m"(*state) : "memory");
  return result->ok;
}

void reset(const Job& job, ArmState* state) {
  state->out = job.seed;
  state->work = {};
  state->work.meb_proposal_supports = job.proposal_start;
}

bool equal_result(const SilentIncidenceResult& a, const SilentIncidenceResult& b) {
  return a.status == b.status && std::strcmp(a.reason, b.reason) == 0 &&
      same_stats(a.stats, b.stats) && same_events(a.events, b.events);
}

std::string route(const Job& job, const ArmState& before, const ArmState& after, bool ok) {
  if (before.out.stats.meb_supports >= job.legacy_cap) return "legacy_guard";
  const u64 certified = after.work.certified - before.work.certified;
  const u64 fallback = after.work.fallback - before.work.fallback;
  check(certified <= 1 && fallback <= 1 && certified + fallback == 1, "cost.exclusive_route");
  if (certified) return ok ? "certificate_accepted" : "certificate_legacy_refused";
  if (before.work.meb_proposal_supports >= job.proposal_cap) return "initial_P_fallback";
  // The present NoObserver/Trace interfaces do not distinguish every cause.
  // Exhaustion at the end is not proof that exhaustion caused this fallback.
  return "fallback_unattributed";
}

void append_job(std::vector<Job>* jobs, const std::vector<Fixture>& all,
                const Order& order, size_t order_id, u64 rank, u64 l, u64 p,
                u64 c = 0, u64 proposed = 0, size_t pivot = 16,
                const char* cohort = "main", size_t steps = 1, size_t repeats = 1) {
  check(order.index < all.size() && steps >= 1 && steps <= 4, "cost.job_domain");
  Job job;
  job.id = jobs->size(); job.fixture_id = order.index; job.order_id = order_id;
  job.order = order; job.rank = rank; job.legacy_cap = l; job.proposal_cap = p;
  job.legacy_start = c; job.proposal_start = proposed; job.pivot_cap = pivot;
  job.steps = steps; job.repeats = repeats; job.cohort = cohort;
  job.seed = State(c, proposed).reference;
  jobs->push_back(std::move(job));
}

std::vector<Job> jobs(const std::vector<Fixture>& all, const std::vector<Order>& ordered,
                      const std::vector<u64>& ranks) {
  std::vector<Job> result;
  for (size_t i = 0; i < ordered.size(); ++i)
    for (const u64 p : {u64{0},u64{1},u64{4},u64{5},u64{15},u64{16},u64{25},u64{401}})
      for (const u64 l : {ranks[i]-1,ranks[i],ranks[i]+1})
        append_job(&result, all, ordered[i], i, ranks[i], l, p);
  check(result.size() == 9216, "cost.main_inventory");

  // Narrow, explicit port of the pinned donor boundaries(): its interface does
  // not expose a case list. The entire aggregate is checked against that donor
  // below; any donor edit still requires a source diff and fresh runner pin.
  const u64 maximum = std::numeric_limits<u64>::max();
  for (const size_t index : {size_t{0},size_t{1},size_t{2},size_t{174},size_t{175}}) {
    const size_t i = 2 * index;
    const u64 r = ranks[i];
    for (const u64 p : {u64{0},u64{1},u64{401}})
      for (const u64 l : {7+r-1,7+r,7+r+1})
        append_job(&result, all, ordered[i], i, r, l, p, 7, 0, 16, "boundary");
    for (const auto& edge : std::array<std::pair<u64,u64>,5>{{
        {0,0},{maximum,maximum},{maximum,maximum-1},{maximum-1,maximum},{maximum-4,maximum}}})
      for (const u64 p : {u64{0},u64{401}})
        append_job(&result, all, ordered[i], i, r, edge.second, p, edge.first, 0, 16, "boundary");
    for (const auto& edge : std::array<std::pair<u64,u64>,3>{{
        {maximum,maximum},{maximum-1,maximum},{5,4}}})
      append_job(&result, all, ordered[i], i, r, 551, edge.second, 0, edge.first, 16, "boundary");
    append_job(&result, all, ordered[i], i, r, 551, 401, 0, 0, 0, "boundary");
  }
  // Same small triangle as the donor boundary, separate from its 176 main scenes.
  const Order triangle{176, all[176].sites};
  append_job(&result, all, triangle, 384, 4, 12, 7, 0, 0, 16, "cumulative_P7", 4);
  append_job(&result, all, triangle, 384, 4, 8, 0, 0, 0, 16, "cumulative_P0", 2);
  for (const size_t pivot : {size_t{17},std::numeric_limits<size_t>::max()})
    append_job(&result, all, ordered[4], 4, ranks[4], 551, 401, 0, 0, pivot, "boundary");
  u64 boundary_calls = 0;
  for (const auto& job : result) if (std::strcmp(job.cohort, "main") != 0) boundary_calls += job.steps;
  check(boundary_calls == 123 && result.size() == 9335, "cost.boundary_inventory");
  for (size_t i = 0; i < 2; ++i)
    for (const u64 p : {u64{0},u64{1},u64{401}})
      for (const u64 l : {u64{1},u64{2}})
        append_job(&result, all, ordered[i], i, ranks[i], l, p, 0, 0, 16,
                   "immediate_q2", 1, kPairRepeats);
  check(result.size() == 9347, "cost.total_jobs");
  return result;
}

void equal_metrics(const Metrics& a, const Metrics& b) {
  // Explicit fields, no struct-padding memcmp. n/named-fast arrays are judged
  // by the donor too; its named counters are not inferred from total successes.
  check(a.main == b.main && a.boundary == b.boundary && a.pilots == b.pilots &&
      a.causal == b.causal && a.forms == b.forms && a.searches == b.searches &&
      a.legacy_charges == b.legacy_charges && a.fallback_candidates == b.fallback_candidates &&
      a.certified == b.certified && a.fallback == b.fallback && a.complete == b.complete &&
      a.degenerate == b.degenerate && a.capped == b.capped &&
      a.q4_two_pivots == b.q4_two_pivots && a.q4_high_limb == b.q4_high_limb &&
      a.exhausted_fallback == b.exhausted_fallback && a.initial_p_fallback == b.initial_p_fallback &&
      a.shell_fallback == b.shell_fallback && a.forced_fallback == b.forced_fallback &&
      a.direct_form_checks == b.direct_form_checks && a.direct_form_rejected == b.direct_form_rejected &&
      a.fast_q == b.fast_q && a.n_seen == b.n_seen && a.named_fast == b.named_fast,
      "cost.donor_boundary_metrics");
}

void print_case(const Job& job, size_t step, const Fixture& f, const ArmState& before,
                const ArmState& after, const Outcome& result) {
  const u64 dc = after.out.stats.meb_supports - before.out.stats.meb_supports;
  const u64 dp = after.work.meb_proposal_supports - before.work.meb_proposal_supports;
  const u64 fallback = after.work.fallback - before.work.fallback;
  std::printf("{\"kind\":\"case\",\"id\":%zu,\"step\":%zu,\"cohort\":\"%s\","
      "\"scene\":%zu,\"order\":%zu,\"n\":%zu,\"ok\":%u,"
      "\"reason\":\"%s\",\"route\":\"%s\"",
      job.id,step,job.cohort,job.fixture_id,job.order_id,f.n,
      unsigned(result.ok),after.out.reason,route(job,before,after,result.ok).c_str());
  if (after.out.status == SilentIncidenceStatus::kResourceExhausted)
    std::printf(",\"q_result\":null");
  else std::printf(",\"q_result\":%u",unsigned(result.ball.q));
  counter("R",job.rank); counter("L",job.legacy_cap); counter("P",job.proposal_cap);
  counter("c0",before.out.stats.meb_supports); counter("p0",before.work.meb_proposal_supports);
  counter("pivot_cap",job.pivot_cap); counter("legacy_delta",dc); counter("proposal_delta",dp);
  counter("actual_F_fallback_candidates",fallback ? dc : 0);
  counter("pivots_delta",after.work.pivots-before.work.pivots);
  counter("certified_delta",after.work.certified-before.work.certified); counter("fallback_delta",fallback);
  counter("terminal_hash",terminal_hash(result.ok,after.out,result.ball));
  counter("work_hash",work_hash(after.work)); std::puts("}");
}

void qualify(std::vector<Job>* jobs, const std::vector<Fixture>& all,
             const std::vector<Order>& ordered, const std::vector<u64>& ranks,
             CallLedger* ledger, bool before_timing) {
  Metrics boundary_metrics, main_metrics, pair_metrics;
  for (auto& job : *jobs) {
    State traced(job.legacy_start, job.proposal_start);
    ArmState f, dual;
    reset(job, &f); reset(job, &dual);
    Metrics* metrics = std::strcmp(job.cohort,"main") == 0 ? &main_metrics :
        (std::strcmp(job.cohort,"immediate_q2") == 0 ? &pair_metrics : &boundary_metrics);
    for (size_t step = 0; step < job.steps; ++step) {
      const ArmState previous = dual;
      const u64 old_fallback = traced.work.fallback;
      ledger->reserve(6);  // F+Trace(+fallback), F+NoObserver(+fallback).
      compare<false>(all[job.fixture_id],job.order,job.legacy_cap,job.proposal_cap,
          &traced,metrics,metrics == &boundary_metrics,job.pivot_cap);
      ledger->add(2 + traced.work.fallback - old_fallback);
      Outcome rf{false,sentinel()}, rd{false,sentinel()};
      invoke_f(all[job.fixture_id],job,&f,&rf);
      invoke_dual(all[job.fixture_id],job,&dual,&rd);
      ledger->add(2 + dual.work.fallback - previous.work.fallback);
      check(rf.ok == rd.ok && equal_result(f.out,dual.out) && same_ball(rf.ball,rd.ball),
            "cost.nominal_full_terminal");
      check(equal_result(traced.reference,f.out) && equal_result(traced.actual,dual.out) &&
            same_work(traced.work,dual.work) && metrics->causal == 0,
            "cost.Trace_and_NoObserver_agree");
      const u64 th = terminal_hash(rd.ok,dual.out,rd.ball), wh = work_hash(dual.work);
      if (before_timing) {
        if (step == 0 && (rd.ok || dual.out.status == SilentIncidenceStatus::kUnsupportedDegeneracy))
          job.first_result_q = rd.ball.q;
        job.expected_terminal.push_back(th); job.expected_work.push_back(wh);
        const auto r = route(job,previous,dual,rd.ok);
        job.route += (step ? "+" : "") + r;
        const std::string terminal = rd.ok ? "success" :
            (dual.out.status == SilentIncidenceStatus::kResourceExhausted ? "legacy_refused" : "shell_refused");
        job.terminals += (step ? "+" : "") + terminal;
        print_case(job,step,all[job.fixture_id],previous,dual,rd);
      } else {
        check(th == job.expected_terminal.at(step) && wh == job.expected_work.at(step),
              "cost.terminal_changed_after_timing");
      }
    }
  }
  Metrics donor;
  ledger->reserve(3 * 123);
  boundaries<false>(all,ordered,ranks,&donor);
  ledger->add(2 * 123 + donor.fallback);
  equal_metrics(boundary_metrics,donor);
  check(main_metrics.main == 9216 && pair_metrics.main == 12 &&
        main_metrics.named_fast == std::array<u64,3>{8,16,52}, "cost.nominal_named_floors");
}

std::string group_key(const Job& job, size_t n) {
  return std::string(job.cohort)+"/n="+std::to_string(n)+"/qref="+std::to_string(job.reference_q)+
      "/P="+std::to_string(job.proposal_cap)+"/L="+std::to_string(job.legacy_cap)+
      "/c="+std::to_string(job.legacy_start)+"/p="+std::to_string(job.proposal_start)+
      "/pivot="+std::to_string(job.pivot_cap)+"/terminal="+job.terminals+"/route="+job.route;
}

void assign_reference_q(std::vector<Job>* jobs) {
  // Annotation read from the already qualified P0/L=R result, never injected
  // into a proposal and never obtained by extra hidden calibration calls.
  std::array<u8,385> q{};
  for (size_t i = 0; i < 384; ++i) {
    const Job& job = (*jobs)[24*i+1];  // P0, L=R; same donor order.
    check(job.first_result_q >= 2 && job.first_result_q <= 4, "cost.reference_q_annotation");
    q[i] = job.first_result_q;
  }
  for (const auto& job : *jobs) if (std::strcmp(job.cohort,"cumulative_P7") == 0) {
    check(job.first_result_q == 3, "cost.cumulative_triangle_q");
    q[384] = job.first_result_q;
  }
  check(q[384] == 3, "cost.missing_triangle_annotation");
  for (auto& job : *jobs) job.reference_q = q.at(job.order_id);
}

struct Digests { u64 terminal = kHashSeed, work = kHashSeed; };

Digests expected_digest(const std::vector<size_t>& ids, const std::vector<Job>& jobs,
                        bool dual) {
  Digests value;
  for (const size_t id : ids) {
    const auto& job = jobs[id];
    proposal::Work unmodified;
    unmodified.meb_proposal_supports = job.proposal_start;
    for (size_t repeat = 0; repeat < job.repeats; ++repeat)
      for (size_t step = 0; step < job.steps; ++step) {
        mix(&value.terminal,job.expected_terminal.at(step));
        mix(&value.work,dual ? job.expected_work.at(step) : work_hash(unmodified));
      }
  }
  return value;
}

void timed_group(const std::string& key, const std::vector<size_t>& ids,
                 const std::vector<Job>& jobs, const std::vector<Fixture>& all,
                 bool dual, size_t pass, bool warmup, u64 clock_tick_ns, CallLedger* ledger) {
  u64 calls = 0;
  for (const size_t id : ids) calls += jobs[id].steps * jobs[id].repeats;
  ledger->reserve(calls * (dual ? 2 : 1));
  const Digests expected = expected_digest(ids,jobs,dual);
  // Allocate both event-sentinel slots outside timing. State assignment/reset,
  // barriers and capture remain inside and are explicitly part of this cost.
  ArmState state;
  state.out.events.reserve(2);
  Digests observed;
  u64 nested = 0;
  Invoke invoke = dual ? invoke_dual : invoke_f;
  asm volatile("" : "+r"(invoke) : : "memory");
  const auto start = Clock::now();
  for (const size_t id : ids) {
    const Job& job = jobs[id];
    const Fixture& f = all[job.fixture_id];
    for (size_t repeat = 0; repeat < job.repeats; ++repeat) {
      reset(job,&state);  // A new attempt, never a reset between cumulative steps.
      for (size_t step = 0; step < job.steps; ++step) {
        Outcome result{false,sentinel()};
        const u64 prior_fallback = state.work.fallback;
        invoke(f,job,&state,&result);
        mix(&observed.terminal,terminal_hash(result.ok,state.out,result.ball));
        mix(&observed.work,work_hash(state.work));
        nested += state.work.fallback - prior_fallback;
        asm volatile("" : : "m"(observed), "m"(state), "m"(result) : "memory");
      }
    }
  }
  const auto finish = Clock::now();
  asm volatile("" : : "m"(observed) : "memory");
  ledger->add(calls + nested);
  check(observed.terminal == expected.terminal && observed.work == expected.work,
        "cost.timed_result_capture");
  check(nested <= (dual ? calls : 0), "cost.nested_call_count");
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
  check(elapsed >= 0, "cost.monotone_clock");
  std::printf("{\"kind\":\"timing\",\"group\":\"%s\",\"arm\":\"%s\","
      "\"pass\":%zu,\"warmup\":%s",key.c_str(),dual ? "dual" : "F",pass,warmup ? "true" : "false");
  counter("calls",calls); counter("nested_F_calls",nested); counter("elapsed_ns",static_cast<u64>(elapsed));
  counter("terminal_hash",observed.terminal); counter("work_hash",observed.work);
  std::printf(",\"short_batch\":%s}\n",static_cast<u64>(elapsed) < 100*clock_tick_ns ? "true" : "false");
}

int measure(const char* geometry_receipt_sha) {
  CallLedger ledger;
  const u64 ordinals = all_ordinals();
  auto all = fixtures();
  const auto ordered = orders(all);
  std::vector<u64> ranks;
  Metrics pilots;
  for (const auto& order : ordered) {
    ledger.reserve(1);
    ranks.push_back(reference_rank(all[order.index],order,&pilots));
    ledger.add(1);
  }
  check(pilots.pilots == 384, "cost.rank_inventory");
  all.push_back(fixture({{0,0,0},{2,2,0},{2,0,2}}));
  auto scheduled = jobs(all,ordered,ranks);
  u64 pass_calls = 0;
  for (const auto& job : scheduled) pass_calls += job.steps*job.repeats;
  check(pass_calls == 58491 && 3*pass_calls*(kWarmups+kPasses)+120000 < kMaxCalls,
        "cost.worst_case_inventory_bound");
  std::printf("{\"kind\":\"header\",\"schema\":\"mhgp7-private-dual-budget-cost-v1\","
      "\"geometry_receipt_sha256\":\"%s\",\"public_status\":\"not_claimed\","
      "\"scenes\":176,\"orders\":384,\"boundary_calls\":123,\"jobs\":9347,"
      "\"NoObserver\":true,\"capture_included\":true",geometry_receipt_sha);
  counter("ordinals",ordinals); counter("calls_per_arm_pass",pass_calls); std::puts("}");
  qualify(&scheduled,all,ordered,ranks,&ledger,true);
  assign_reference_q(&scheduled);
  std::map<std::string,std::vector<size_t>> groups;
  for (const auto& job : scheduled) groups[group_key(job,all[job.fixture_id].n)].push_back(job.id);
  for (const auto& [key,ids] : groups) {
    std::printf("{\"kind\":\"group\",\"group\":\"%s\",\"jobs\":[",key.c_str());
    for (size_t i = 0; i < ids.size(); ++i) std::printf("%s%zu",i ? "," : "",ids[i]);
    std::puts("]}");
  }
  // Fixed clock diagnostic, not an adaptive repetition pilot or time subtraction.
  u64 tick = std::numeric_limits<u64>::max();
  for (size_t i = 0; i < 128; ++i) {
    const auto a = Clock::now(), b = Clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count();
    if (ns > 0) tick = std::min(tick,static_cast<u64>(ns));
  }
  check(tick != std::numeric_limits<u64>::max() && tick <= 1000000000, "cost.clock_resolution");
  for (size_t stage = 0; stage < 2; ++stage) {
    const bool warmup = stage == 0;
    const size_t passes = warmup ? kWarmups : kPasses;
    for (size_t pass = 1; pass <= passes; ++pass)
      for (const auto& [key,ids] : groups)
        for (size_t arm = 0; arm < 2; ++arm) {
          const bool dual = pass % 2 ? arm == 1 : arm == 0;
          timed_group(key,ids,scheduled,all,dual,pass,warmup,tick,&ledger);
        }
  }
  qualify(&scheduled,all,ordered,ranks,&ledger,false);
  std::printf("{\"kind\":\"terminal\",\"status\":\"completed\",\"public_status\":\"not_claimed\"");
  counter("helper_entries",ledger.entries); counter("max_helper_entries",kMaxCalls);
  counter("clock_tick_ns",tick); counter("groups",groups.size());
  counter("warmups",kWarmups); counter("measured_passes",kPasses);
  std::puts(",\"full_terminals_equal_before_after\":true,\"timed_captures_equal\":true}");
  return 0;
}

}  // namespace cost
}  // namespace

int main(int argc, char** argv) {
  // The pinned Python runner owns receipt validation and the 120s deadline.
  // A raw invocation is not an authorized/certified measurement artifact.
  if (argc != 3 || std::strcmp(argv[1],"--measure") != 0 || std::strlen(argv[2]) != 64) return 2;
  for (const char* p = argv[2]; *p; ++p) if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f'))) return 2;
  try {
    return cost::measure(argv[2]);
  } catch (const std::exception& error) {
    std::fprintf(stderr,"meb_cost=failed reason=%s\n",error.what());
    return 1;
  }
}
