// Standalone FULL horizontal probe; mono by default, pipeline threads opt-in.
// FULL order builders and the K loop remain sequential; not the production CLI.
// Public status remains not_claimed. No Gamma core, archive, vertical maps,
// masses or geometric oracle is built. Conditional authority is the public
// full_gabriel.hpp authority, not an independently certified catalogue.
//
// Fixed family, no arbitrary operation quotas. Explicit finite P remains an
// algorithmic choice; P=unlimited removes that quota up to checked u64 range.
// The 8 GiB budget guards named logical payloads, not allocator capacities/RSS.
// Prefilter and census admission follow the nominal direct census (one BallData
// destination), not the historical two-BallData proxy retained in run.hpp.
// RLIMIT_AS separately lowers the
// process address-space soft limit to at most 26 GiB (never raises it).
// Measurements are directly monitored; an interrupted process or a missing
// terminal JSONL record is NOT a successful run.
// Semantic SHA256 fingerprints compare labelled horizontal forests, NOT the
// completeness of Gabriel catalogues. Final roots/coverage are not an oracle.
// Both policies must be measured with THIS digest-enabled probe. No latency
// ratio against the historical digest-free probe is admissible.
// Input size is not restricted to a benchmark whitelist.
#include <sys/resource.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/forest/full_gabriel.hpp"
#include "../src/pipeline/run.hpp"
#include "full_gabriel_semantic_digest.hpp"
#include "full_gabriel_probe_limits.hpp"

#ifdef MHGP7_TESTING
#error "full_gabriel_lazy_probe must use product headers without MHGP7_TESTING"
#endif

namespace {
using namespace mhgp7;
using Clock = std::chrono::steady_clock;
constexpr const char* kSchema = "mhgp7-full-gabriel-probe-v5";
constexpr const char* kParallelSchema = "mhgp7-full-gabriel-probe-v6";
constexpr u64 kGiB = 1ull << 30;
constexpr u64 kVmBytes = 26 * kGiB;
constexpr u64 kPayloadBytes = 8 * kGiB;
// The per-call native pivot limit terminates proposal search with fallback;
// it is not a probe work quota and is deliberately unchanged.
static_assert(meb_proposal_detail::kMaxPivots == 16);

struct Options {
  u64 n = 0, s = 0, kmax = 0, cache_entries = 0;
  u64 meb_proposal_supports = 0;
  full_probe_limits::ProposalKind proposal_kind = full_probe_limits::ProposalKind::kDisabled;
  bool lazy = false, policy_set = false, cache_set = false;
  bool meb_proposal_set = false;
  int threads = 1;
  bool threads_set = false;
};

const char* alias_policy(const Options& o) {
  return o.lazy ? kFullGabrielLazyAliases : kFullGabrielEagerAliases;
}

bool parse_options(int argc, char** argv, Options& o, const full_probe_limits::Policy& limits) {
  // n/s/kmax/policy/P are mandatory and unique, including explicit P=0.
  // Cache capacity must be explicit for lazy (zero is permitted), and is
  // forbidden for eager. Storage is memory/representation bounded; finite P
  // exhaustion falls back to F. Numeric MAX and literal unlimited are distinct
  // declared modes, even though both use the same representable engine ceiling.
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--threads=", 10) == 0) {
      if (o.threads_set) return false;
      o.threads_set = true;
      u64 parsed = 0;
      if (!full_probe_limits::decimal(argv[i] + 10, parsed) || parsed == 0 ||
          parsed > static_cast<u64>(std::numeric_limits<int>::max())) return false;
      o.threads = static_cast<int>(parsed);
      continue;
    }
    if (std::strncmp(argv[i], "--alias-policy=", 15) == 0) {
      if (o.policy_set) return false;
      o.policy_set = true;
      if (std::strcmp(argv[i] + 15, "lazy") == 0) o.lazy = true;
      else if (std::strcmp(argv[i] + 15, "eager") != 0) return false;
      continue;
    }
    const char* value = nullptr;
    u64* destination = nullptr;
    bool zero_allowed = false;
    if (std::strncmp(argv[i], "--n=", 4) == 0) {
      destination = &o.n; value = argv[i] + 4;
    } else if (std::strncmp(argv[i], "--s=", 4) == 0) {
      destination = &o.s; value = argv[i] + 4;
    } else if (std::strncmp(argv[i], "--kmax=", 7) == 0) {
      destination = &o.kmax; value = argv[i] + 7;
    } else if (std::strncmp(argv[i], "--cache-entries=", 16) == 0) {
      if (o.cache_set) return false;
      o.cache_set = true; zero_allowed = true;
      destination = &o.cache_entries; value = argv[i] + 16;
    } else if (std::strncmp(argv[i], "--meb-proposal-supports=", 24) == 0) {
      if (o.meb_proposal_set) return false;
      o.meb_proposal_set = true;
      if (!full_probe_limits::proposal(argv[i] + 24, o.meb_proposal_supports, o.proposal_kind)) return false;
      continue;
    } else return false;
    if (*destination != 0 || *value == '\0') return false;
    u64 parsed = 0;
    if (!full_probe_limits::decimal(value, parsed) || (!zero_allowed && parsed == 0)) return false;
    *destination = parsed;
  }
  return limits.valid && o.n >= 2 && o.n <= limits.full.max_points &&
      (o.s == 8 || o.s == 10 || o.s == 12) && (o.kmax == 5 || o.kmax == 10) &&
      o.policy_set && o.cache_set == o.lazy && o.cache_entries <= limits.max_cache_entries && o.meb_proposal_set;
}

enum Stage : size_t {
  kInput, kIndex, kGenerate, kRle, kPrefilter, kCensus, kRegularity,
  kCount, kExpand, kFull, kRead, kDigest, kRelease, kStageCount
};
constexpr const char* kStageNames[kStageCount] = {
    "input", "index", "generation", "rle", "prefilter", "census",
    "regularity", "count", "expand", "full", "read", "digest", "release"};

struct State {
  Options options;
  Stage stage = kInput;
  u64 order = 0, completed_orders = 0, smax = 0, kmax = 0;
  u64 vm_soft_bytes = 0, raw = 0, unique = 0, census_balls = 0;
  u64 candidate_capacity = 0, ball_capacity = 0, extra_shell = 0;
  u64 expected_pair_mass = 0, rle_workers = 0;
  bool ledger_closed = false, regular = false;
  GenerateStats gen;
  ExpandStats expand;
  FullGabrielStats last_full;
  std::string input_digest, certificate_digest;
  std::vector<std::string> order_digests;
  std::array<double, kStageCount> stage_ms{};
  double output_ms = 0;
};

struct Timer {
  State& state;
  Stage stage;
  Clock::time_point begin = Clock::now();
  Timer(State& s, Stage p) : state(s), stage(p) { state.stage = p; }
  ~Timer() { state.stage_ms[stage] += run_detail::ms(begin); }
};

struct Failure { PipelineStatus status; const char* reason; };
[[noreturn]] void fail(PipelineStatus status, const char* reason) { throw Failure{status, reason}; }

const char* outcome(PipelineStatus s) {
  switch (s) {
    case PipelineStatus::kCompleteRegular: return "complete_relative";
    case PipelineStatus::kResourceExhausted: return "resource_exhausted";
    case PipelineStatus::kUnsupportedDegeneracy: return "unsupported_degeneracy";
    case PipelineStatus::kInvalidInput: return "invalid_input";
    case PipelineStatus::kInvariantViolated: return "invariant_violated";
  }
  return "unknown_status";
}

PipelineStatus map_full_status(FullGabrielStatus s) {
  switch (s) {
    case FullGabrielStatus::kCompleteRelative: return PipelineStatus::kCompleteRegular;
    case FullGabrielStatus::kResourceExhausted: return PipelineStatus::kResourceExhausted;
    case FullGabrielStatus::kUnsupportedDegeneracy: return PipelineStatus::kUnsupportedDegeneracy;
    case FullGabrielStatus::kInvalidInput: return PipelineStatus::kInvalidInput;
    case FullGabrielStatus::kInvariantViolated: return PipelineStatus::kInvariantViolated;
  }
  return PipelineStatus::kInvariantViolated;
}

// No owning C++ output buffer; stdio may itself allocate. Escape failures too.
void quoted(const char* p) {
  std::putchar('"');
  for (; *p; ++p) {
    const auto c = static_cast<unsigned char>(*p);
    if (c == '"' || c == '\\') { std::putchar('\\'); std::putchar(c); }
    else if (c < 32) std::printf("\\u%04x", static_cast<unsigned>(c));
    else std::putchar(c);
  }
  std::putchar('"');
}

struct Json {
  explicit Json(const char* type, const Options* options = nullptr) {
    const bool parallel_profile = options != nullptr && options->threads_set;
    std::fputs("{\"schema\":", stdout); quoted(parallel_profile ? kParallelSchema : kSchema); text("type", type);
    text("limits_profile", full_probe_limits::kProfile);
    text("successor_accounting", kFullGabrielSuccessorAccounting);
    text("meb_accounting", kFullGabrielMebAccounting);
    text("census_payload_accounting", full_probe_limits::kCensusPayloadAccounting);
    if (parallel_profile) {
      text("parallel_profile", "pipeline_workers_full_order_serial_v1");
      number("pipeline_threads", static_cast<u64>(options->threads));
      number("full_order_builder_threads", 1);
      text("order_schedule", "sequential_k1_to_kmax");
    }
  }
  void key(const char* name) { std::putchar(','); quoted(name); std::putchar(':'); }
  void text(const char* name, const char* value) { key(name); quoted(value); }
  void number(const char* name, u64 value) { key(name); std::printf("%" PRIu64, value); }
  void real(const char* name, double value) { key(name); std::printf("%.6f", value); }
  void boolean(const char* name, bool value) { key(name); std::fputs(value ? "true" : "false", stdout); }
  void end() { std::fputs("}\n", stdout); }
};

void checked_flush() {
  if (std::fflush(stdout) != 0 || std::ferror(stdout))
    fail(PipelineStatus::kInvariantViolated, "probe_output_failed");
}

void lower_vm_limit(State& state) {
  static_assert(sizeof(rlim_t) >= sizeof(u64));
  struct rlimit limit {};
  if (getrlimit(RLIMIT_AS, &limit) != 0)
    fail(PipelineStatus::kResourceExhausted, "probe_vm_limit_unreadable");
  if (limit.rlim_cur == RLIM_INFINITY || limit.rlim_cur > static_cast<rlim_t>(kVmBytes)) {
    limit.rlim_cur = static_cast<rlim_t>(kVmBytes);
    if (setrlimit(RLIMIT_AS, &limit) != 0)
      fail(PipelineStatus::kResourceExhausted, "probe_vm_limit_unset");
  }
  if (getrlimit(RLIMIT_AS, &limit) != 0 || limit.rlim_cur == RLIM_INFINITY ||
      limit.rlim_cur > static_cast<rlim_t>(kVmBytes))
    fail(PipelineStatus::kResourceExhausted, "probe_vm_limit_unverified");
  state.vm_soft_bytes = static_cast<u64>(limit.rlim_cur);
}

void print_config(const State& state, const RunOptions& opt, const FullGabrielLimits& c,
                  const full_probe_limits::Policy& limits) {
  Json j("configuration", &state.options);
  j.text("phase", "exploration_v7_hors_registre");
  j.text("backend", "cpu_reference"); j.text("profile", "quantized_u16_input_only");
  j.text("mode", "audit_independant_math_and_architecture"); j.text("public_status", "not_claimed");
  j.text("authority", kFullGabrielAuthority);
  j.text("family", "uniform"); j.number("seed", 3); j.number("coord", 65536);
  j.text("input_generator", "make_family_input_CloudFamily_kUniform");
  j.text("input_digest_kind", "sha256_FULLv1_labelled_u16_input");
  j.text("certificate_digest_kind", full_probe_digest::kKind);
  j.text("s_comparison_scope", "semantic_labelled_horizontal_forests_not_catalogue_completeness");
  j.text("alias_policy", alias_policy(state.options));
  j.number("cache_entries", state.options.cache_entries);
  j.number("max_cache_entries", limits.max_cache_entries);
  j.text("meb_proposal_budget_kind", full_probe_limits::proposal_kind(state.options.proposal_kind));
  j.text("storage_limit_kind", full_probe_limits::kStorageKind);
  j.boolean("legacy_F_fold_guard_applied", false);
  j.number("work_counter_ceiling", full_probe_limits::kCounterCeiling);
  j.number("storage_size_max", full_probe_limits::kSizeMax);
  j.number("storage_difference_max", full_probe_limits::kDifferenceMax);
  j.text("cache_policy", state.options.lazy ? "first_C_resolved_nonminimum_strict_facets" : "not_applicable");
  j.number("digest_scratch_bytes_per_node", full_probe_digest::kScratchBytesPerNode);
  j.number("digest_scratch_bytes_per_parent", sizeof(FullNodeId));
  j.number("max_digest_scratch_logical_bytes", limits.max_digest_scratch_logical_bytes);
  j.number("input_digest_scratch_bytes_per_point", sizeof(size_t));
  j.text("digest_scratch_scope", "additional_logical_sizes_not_allocator_capacity_or_RSS_bound");
  j.text("digest_timing_scope", "included_in_elapsed_and_stage_digest_ms_not_subtracted");
  j.text("read_kind", "terminal_closed_roots_and_point_coverage_sentinel_not_oracle");
  j.number("n", state.options.n); j.number("s", state.options.s);
  j.number("kmax_requested", state.options.kmax); j.number("kmax_effective", state.kmax);
  j.number("catalogue_cardinality_max", state.smax); j.number("threads", static_cast<u64>(state.options.threads));
  j.boolean("gpu", false); j.boolean("archive", false); j.boolean("vertical", false);
  j.number("max_raw_candidates", opt.max_raw_candidates);
  j.number("effective_raw_cap", effective_raw_cap(opt));
  j.number("candidate_fusion_cap_2e", budget_fusion_cap(opt));
  j.number("named_payload_budget_bytes", opt.memory_budget_bytes);
  j.number("vm_soft_limit_bytes", state.vm_soft_bytes);
  j.number("shell_cap", opt.shell_cap);
  j.number("pretest_query_min_points", opt.pretest_query_min_points);
  j.number("cell_grid_min_sites", opt.cell_grid_min_sites);
  j.number("max_wave_tasks", kMaxWaveTasks); j.number("max_alive_rects", kMaxAliveRects);
  j.number("max_points_per_order", c.max_points);
  j.number("max_input_records_per_order", c.max_input_records);
  j.number("max_aliases_per_order", c.max_aliases);
  j.number("max_face_visits_per_order", c.max_face_visits);
  j.number("max_portal_requests_per_order", c.max_portal_requests);
  j.number("max_chain_steps_per_order", c.max_chain_steps);
  j.number("max_meb_calls_per_order", c.max_meb_calls);
  j.number("max_query_nodes_per_order", c.max_query_nodes);
  j.number("max_meb_supports_per_order", c.max_meb_supports);
  j.number("max_meb_proposal_supports_per_order", c.max_meb_proposal_supports);
  j.number("max_successor_steps_per_order", c.max_successor_steps);
  j.number("max_certificate_batches_per_order", c.certificate.max_batches);
  j.number("max_certificate_nodes_per_order", c.certificate.max_nodes);
  j.number("max_certificate_parent_refs_per_order", c.certificate.max_parent_refs);
  j.number("max_read_point_refs_per_order", limits.max_read_point_refs);
  j.number("sizeof_input_point", sizeof(InputPoint));
  j.number("sizeof_ball_candidate", sizeof(BallCandidate)); j.number("sizeof_ball_data", sizeof(BallData));
  j.number("sizeof_survivor", sizeof(Survivor));
  j.number("sizeof_forest_event", sizeof(ForestEvent)); j.number("sizeof_facet_key", sizeof(FacetKey));
  j.number("sizeof_full_node", sizeof(FullNode));
  j.number("sizeof_full_batch", sizeof(FullBatch));
  j.number("sizeof_full_record", sizeof(full_gabriel_detail::Record));
  j.number("sizeof_alias_entry_payload", sizeof(full_probe_limits::AliasEntryPayload));
  j.number("sizeof_full_parent_id", sizeof(FullNodeId));
  j.number("sizeof_point_id", sizeof(PointId)); j.end(); checked_flush();
}

void full_work(Json& j, const FullGabrielStats& s) {
  j.number("input_records", s.input_records); j.number("aliases", s.aliases);
  j.number("face_visits", s.face_visits); j.number("alias_hits", s.alias_hits);
  j.number("portal_requests", s.portal_requests); j.number("chain_steps", s.chain_steps);
  j.number("terminal_direct", s.terminal_direct); j.number("max_chain_length", s.max_chain_length);
  j.number("normalized_anchors", s.normalized_anchors); j.number("successor_steps", s.successor_steps);
  j.number("no_op_connections", s.no_op_connections); j.number("meb_calls", s.meb_calls);
  j.number("geometry_meb_calls", s.geometry.meb_calls);
  j.number("meb_supports", s.geometry.meb_supports);
  j.number("query_nodes", s.geometry.query_nodes); j.number("query_leaves", s.geometry.query_leaves);
  j.number("query_range_skips", s.geometry.query_range_skips);
  j.number("minimum_lookups", s.minimum_lookups); j.number("minimum_hits", s.minimum_hits);
  j.number("cache_lookups", s.cache_lookups); j.number("cache_hits", s.cache_hits);
  j.number("cache_inserts", s.cache_inserts); j.number("cache_skips", s.cache_skips);
  j.number("singleton_intruder_resolutions", s.singleton_intruder_resolutions);
  j.number("direct_lookups", s.direct_lookups);
  // meb_supports above remains the reference ordinal prefix c. A below is
  // physical F work; p counts prospectively admitted proposal forms. Never
  // present c as total physical work or add the independent u64 budgets.
  j.number("meb_proposal_supports", s.meb_proposal.meb_proposal_supports);
  j.number("meb_proposal_pivots", s.meb_proposal.pivots);
  j.number("meb_proposal_certified", s.meb_proposal.certified);
  j.number("meb_proposal_fallback", s.meb_proposal.fallback);
  j.number("meb_reference_supports", s.meb_proposal.reference_supports);
}

struct OrderSummary {
  u64 minima = 0, connections = 0, nodes = 0, leaves = 0, parents = 0, roots = 0, points = 0;
  double expand_ms = 0, build_ms = 0, read_ms = 0, digest_ms = 0, release_ms = 0;
  std::string digest;
};

void print_order(State& state, const OrderSummary& s, PipelineStatus status, const char* reason) {
  const auto begin = Clock::now();
  Json j("order", &state.options); j.boolean("provisional", true);
  j.number("k", state.order); j.text("outcome", outcome(status)); j.text("reason", reason);
  j.boolean("whole_tower_authority", false);
  j.text("alias_policy", alias_policy(state.options));
  j.number("cache_entries", state.options.cache_entries);
  j.text("meb_proposal_budget_kind", full_probe_limits::proposal_kind(state.options.proposal_kind));
  j.number("max_meb_proposal_supports_per_order", state.options.meb_proposal_supports);
  j.text("certificate_digest", s.digest.c_str());
  j.number("minimum_catalogue_records", s.minima); j.number("connection_catalogue_records", s.connections);
  j.number("certificate_nodes", s.nodes); j.number("certificate_minima", s.leaves);
  j.number("certificate_parent_refs", s.parents); j.number("terminal_roots", s.roots);
  j.number("terminal_coverage_points", s.points); full_work(j, state.last_full);
  j.real("expand_ms", s.expand_ms); j.real("build_ms", s.build_ms);
  j.real("read_ms", s.read_ms); j.real("digest_ms", s.digest_ms); j.real("release_ms", s.release_ms);
  j.real("rss_mib_sample", run_detail::rss_mb_now()); j.real("hwm_mib_sample", run_detail::vm_hwm_mb_now());
  j.end(); checked_flush(); state.output_ms += run_detail::ms(begin);
}

void check_read(FullCertificateStatus status, const char* reason) {
  if (status == FullCertificateStatus::kResourceExhausted) fail(PipelineStatus::kResourceExhausted, reason);
  if (status != FullCertificateStatus::kOk) fail(PipelineStatus::kInvariantViolated, reason);
}

void run(State& state, const RunOptions& opt, const FullGabrielLimits& caps,
         const full_probe_limits::Policy& limits) {
  std::vector<InputPoint> input;
  {
    Timer timer(state, kInput);
    if (state.options.n > kMaxTreePositions || state.options.n > caps.max_points)
      fail(PipelineStatus::kResourceExhausted, "probe_point_budget");
    input = make_family_input(CloudFamily::kUniform, static_cast<int>(state.options.n), 65536, 3);
    if (input.size() != state.options.n) fail(PipelineStatus::kInvalidInput, "probe_incomplete_family");
    std::string why;
    if (!validate_run_options(input, opt, &why)) fail(PipelineStatus::kInvalidInput, "probe_run_options");
    if (opt.memory_budget_bytes < sizeof(BallCandidate))
      fail(PipelineStatus::kResourceExhausted, "probe_single_candidate_budget");
  }
  {
    Timer timer(state, kDigest);
    state.input_digest = full_probe_digest::input(input);
    state.order_digests.reserve(static_cast<size_t>(state.kmax));
  }
  CloudIndex ix;
  {
    Timer timer(state, kIndex);
    ix = build_cloud_index(input);
    if (!ix.valid) fail(PipelineStatus::kInvalidInput, "probe_invalid_index");
    if (ix.has_duplicate_positions()) fail(PipelineStatus::kUnsupportedDegeneracy, "probe_duplicate_positions");
  }
  std::vector<BallCandidate> candidates;
  {
    Timer timer(state, kGenerate);
    GenerateOptions go;
    go.s = opt.s; go.smax = state.smax; go.threads = opt.threads;
    go.pretest_query_min_points = opt.pretest_query_min_points;
    go.cell_grid_min_sites = opt.cell_grid_min_sites;
    go.e6_probe = opt.e6_probe; go.e3_mode = opt.e3_mode;
    go.max_raw_candidates = effective_raw_cap(opt); go.memory_budget_bytes = opt.memory_budget_bytes;
    generate_candidates(ix, go, &candidates, &state.gen);
    state.raw = candidates.size(); state.candidate_capacity = candidates.capacity();
    // Exactly the run.hpp ordering: typed generation refusal BEFORE ledger.
    switch (state.gen.cap_refus) {
      case kCapRefusNone: break;
      case kCapRefusRawCandidates: fail(PipelineStatus::kResourceExhausted, "probe_raw_candidate_cap");
      case kCapRefusWaveTasks: fail(PipelineStatus::kResourceExhausted, "probe_wave_task_cap");
      case kCapRefusAliveRects: fail(PipelineStatus::kResourceExhausted, "probe_alive_rectangle_cap");
      case kCapRefusFusionBudget: fail(PipelineStatus::kResourceExhausted, "probe_generation_fusion_budget_2e");
      default: fail(PipelineStatus::kInvariantViolated, "probe_unknown_generation_cap");
    }
    const u128 expected = expected_pair_mass(ix);
    if (expected > std::numeric_limits<u64>::max())
      fail(PipelineStatus::kInvariantViolated, "probe_pair_mass_domain");
    state.expected_pair_mass = static_cast<u64>(expected);
    for (size_t q = 0; q < 3; ++q)
      if (state.gen.ledger_emitted_mass[q] + state.gen.ledger_killed_mass[q] != expected)
        fail(PipelineStatus::kInvariantViolated, "probe_pair_mass_ledger");
    state.ledger_closed = true;
    if (state.gen.invariant_jneg) fail(PipelineStatus::kInvariantViolated, "probe_q4_negative_j");
    if (state.raw != state.gen.candidates[0] + state.gen.candidates[1] + state.gen.candidates[2])
      fail(PipelineStatus::kInvariantViolated, "probe_emission_count");
  }
  {
    Timer timer(state, kRle);
    if (!fits_budget(candidates.size(), sizeof(BallCandidate), 2, opt.memory_budget_bytes))
      fail(PipelineStatus::kResourceExhausted, "probe_candidate_sort_budget");
    state.rle_workers = sort_candidates(&candidates, opt.threads);
    deduplicate_candidates(&candidates);
    state.unique = candidates.size(); state.expand.unique_balls = candidates.size();
  }
  std::vector<Survivor> survivors;
  std::vector<BallData> balls;
  {
    Timer timer(state, kPrefilter);
    if (!candidates_capacity_ok(candidates.size()))
      fail(PipelineStatus::kResourceExhausted, "probe_survivor_u32_capacity");
    if (!full_probe_limits::prefilter_payload_fits<BallCandidate, Survivor>(
            candidates.size(), opt.memory_budget_bytes))
      fail(PipelineStatus::kResourceExhausted, "probe_prefilter_survivor_payload_budget");
    prefilter_balls(ix, candidates, state.smax, opt.threads, &survivors, &state.expand);
  }
  {
    Timer timer(state, kCensus);
    if (survivors.size() > candidates.size())
      fail(PipelineStatus::kInvariantViolated, "probe_survivor_count");
    if (!full_probe_limits::census_payload_fits<BallCandidate, Survivor, BallData>(
            candidates.size(), survivors.size(), opt.memory_budget_bytes))
      fail(PipelineStatus::kResourceExhausted, "probe_direct_census_payload_budget");
    const PipelineStatus status = census_balls(ix, candidates, survivors, state.smax, opt.shell_cap,
                                              opt.threads, &balls, &state.expand);
    if (status != PipelineStatus::kCompleteRegular) fail(status, "probe_census_refused");
    state.census_balls = balls.size(); state.ball_capacity = balls.capacity();
    if (balls.size() != survivors.size()) fail(PipelineStatus::kInvariantViolated, "probe_census_count");
  }
  {
    Timer timer(state, kRelease);
    std::vector<Survivor>().swap(survivors);
    std::vector<BallCandidate>().swap(candidates);
  }
  {
    Timer timer(state, kRegularity);
    // Global over all census balls relevant to this declared rank window;
    // NOT a regularity assertion for ungenerated higher-cardinality balls.
    for (const BallData& b : balls) {
      if (b.n_shell != b.arity) { ++state.extra_shell; continue; }
      if (b.arity < 2 || b.arity > 4 || static_cast<u64>(b.n_interior) + b.n_shell > state.smax)
        fail(PipelineStatus::kInvariantViolated, "probe_census_record_domain");
    }
    if (state.extra_shell) fail(PipelineStatus::kUnsupportedDegeneracy, "probe_rank_relevant_extra_shell");
    state.regular = true;
  }
  std::vector<KCount> counts;
  {
    Timer timer(state, kCount);
    counts = count_events_by_k(ix, balls, state.smax - 1, 1);
    u64 total = 0;
    for (u64 k = 1; k < state.smax; ++k) {
      if (counts[k].events > std::numeric_limits<u64>::max() - total)
        fail(PipelineStatus::kResourceExhausted, "probe_catalogue_count_overflow");
      total += counts[k].events;
    }
    if (total != balls.size()) fail(PipelineStatus::kInvariantViolated, "probe_catalogue_partition_count");
    for (u64 k = 1; k <= state.kmax; ++k) {
      const u64 minimum = k == 1 ? 0 : counts[k - 1].events;
      const u64 direct = k < state.smax ? counts[k].events : 0;
      if (minimum > caps.max_input_records || direct > caps.max_input_records - minimum)
        fail(PipelineStatus::kResourceExhausted, "probe_catalogue_input_budget");
      // Sizes only: census + previous catalogue + two copies of expansion.
      // Each term is bounded above before this sum; capacities are not claimed.
      u64 catalogue_records = 0, ball_bytes = 0;
      if (!full_probe_limits::product_sum(minimum, 1, direct, 2, catalogue_records) ||
          !full_probe_limits::product_sum(balls.size(), sizeof(BallData), 0, 0, ball_bytes))
        fail(PipelineStatus::kResourceExhausted, "probe_catalogue_payload_overflow");
      if (ball_bytes > opt.memory_budget_bytes ||
          !fits_budget(catalogue_records, sizeof(ForestEvent), 1, opt.memory_budget_bytes - ball_bytes))
        fail(PipelineStatus::kResourceExhausted, "probe_two_catalogue_payload_budget");
    }
  }
  std::vector<ForestEvent> minimum, direct;
  for (u64 k = 1; k <= state.kmax; ++k) {
    state.order = k; state.last_full = {};
    OrderSummary summary;
    {
      Timer timer(state, kExpand);
      const auto begin = Clock::now();
      minimum = std::move(direct);  // previous direct has cardinality k
      direct = {};
      if (k < state.smax) expand_events_k(ix, balls, k, state.smax - 1, opt.threads, &direct, &state.expand);
      summary.minima = minimum.size(); summary.connections = direct.size();
      if (minimum.size() != (k == 1 ? 0 : counts[k - 1].events) ||
          direct.size() != (k < state.smax ? counts[k].events : 0))
        fail(PipelineStatus::kInvariantViolated, "probe_expanded_catalogue_count");
      summary.expand_ms = run_detail::ms(begin);
    }
    FullGabrielResult result;
    {
      Timer timer(state, kFull);
      const auto begin = Clock::now();
      if (state.options.lazy) {
        result = build_full_gabriel_order_lazy(ix, static_cast<unsigned>(k), minimum, direct, caps,
                                               FullGabrielCacheLimits{state.options.cache_entries});
      } else {
        result = build_full_gabriel_order(ix, static_cast<unsigned>(k), minimum, direct, caps);
      }
      state.last_full = result.stats;
      summary.build_ms = run_detail::ms(begin);
    }
    if (std::strcmp(result.alias_policy, alias_policy(state.options)) != 0)
      fail(PipelineStatus::kInvariantViolated, "probe_alias_policy_mismatch");
    if (std::strcmp(result.successor_accounting, kFullGabrielSuccessorAccounting) != 0)
      fail(PipelineStatus::kInvariantViolated, "probe_successor_accounting_mismatch");
    if (std::strcmp(result.meb_accounting, kFullGabrielMebAccounting) != 0)
      fail(PipelineStatus::kInvariantViolated, "probe_meb_accounting_mismatch");
    if (state.options.lazy && (result.stats.aliases != 0 || result.stats.alias_hits != 0 ||
                              result.stats.cache_inserts > state.options.cache_entries))
      fail(PipelineStatus::kInvariantViolated, "probe_lazy_alias_invariant");
    if (result.status != FullGabrielStatus::kCompleteRelative) {
      const PipelineStatus status = map_full_status(result.status);
      if (result.forest.order() != 0 || !result.forest.nodes().empty() ||
          !result.forest.minima().empty() || !result.forest.parents().empty())
        fail(PipelineStatus::kInvariantViolated, "probe_nonempty_refused_forest");
      print_order(state, summary, status, result.reason);
      fail(status, result.reason);
    }
    {
      Timer timer(state, kRead);
      const auto begin = Clock::now();
      const FullCertificate& forest = result.forest;
      if (forest.order() != k || forest.nodes().empty())
        fail(PipelineStatus::kInvariantViolated, "probe_empty_completed_forest");
      summary.nodes = forest.nodes().size(); summary.leaves = forest.minima().size();
      summary.parents = forest.parents().size();
      const auto roots = full_certificate_roots_at(forest, forest.nodes().back().level, true, caps.certificate.max_nodes);
      check_read(roots.status, roots.reason); summary.roots = roots.values.size();
      if (roots.values.size() != 1) fail(PipelineStatus::kInvariantViolated, "probe_terminal_root_sentinel");
      const auto coverage = full_certificate_coverage(forest, roots.values[0], caps.certificate.max_nodes, limits.max_read_point_refs);
      check_read(coverage.status, coverage.reason); summary.points = coverage.values.size();
      if (coverage.values.size() != input.size())
        fail(PipelineStatus::kInvariantViolated, "probe_terminal_coverage_size");
      for (size_t i = 0; i < input.size(); ++i)
        if (coverage.values[i] != input[i].id)
          fail(PipelineStatus::kInvariantViolated, "probe_terminal_coverage_ids");
      summary.read_ms = run_detail::ms(begin);
    }
    {
      Timer timer(state, kDigest);
      const auto begin = Clock::now();
      summary.digest = full_probe_digest::forest(full_probe_digest::view(result.forest),
          caps.certificate.max_nodes, caps.certificate.max_parent_refs, state.input_digest);
      state.order_digests.push_back(summary.digest);
      summary.digest_ms = run_detail::ms(begin);
    }
    {
      Timer timer(state, kRelease);
      const auto begin = Clock::now();
      result.forest = FullCertificate();
      std::vector<ForestEvent>().swap(minimum);
      summary.release_ms = run_detail::ms(begin);
    }
    ++state.completed_orders;
    print_order(state, summary, PipelineStatus::kCompleteRegular, kFullGabrielAuthority);
  }
  {
    Timer timer(state, kRelease);
    std::vector<ForestEvent>().swap(direct);
    std::vector<BallData>().swap(balls);
  }
  {
    Timer timer(state, kDigest);
    if (state.order_digests.size() != state.kmax)
      fail(PipelineStatus::kInvariantViolated, "probe_incomplete_digest_order_loop");
    state.certificate_digest = full_probe_digest::orders(state.input_digest, state.order_digests);
  }
}

void print_terminal(const State& state, PipelineStatus status, const char* reason, double wall_ms) {
  Json j("terminal", &state.options);
  const bool complete = status == PipelineStatus::kCompleteRegular;
  j.text("terminal_status", complete ? "completed" : "failed");
  j.text("outcome", outcome(status)); j.text("reason", reason); j.text("public_status", "not_claimed");
  j.text("authority", kFullGabrielAuthority);
  j.number("exit_code", static_cast<u64>(status_exit_code(status)));
  j.boolean("complete_requested_horizontal_orders", complete);
  j.boolean("integrated_inter_k_tower", false); j.boolean("certificate_retained", false);
  j.boolean("legacy_F_fold_guard_applied", false);
  j.text("input_digest_kind", "sha256_FULLv1_labelled_u16_input");
  j.text("certificate_digest_kind", full_probe_digest::kKind);
  j.text("input_digest", state.input_digest.c_str());
  j.text("certificate_digest", complete ? state.certificate_digest.c_str() : "");
  j.text("alias_policy", alias_policy(state.options));
  j.number("cache_entries", state.options.cache_entries);
  j.text("meb_proposal_budget_kind", full_probe_limits::proposal_kind(state.options.proposal_kind));
  j.number("max_meb_proposal_supports_per_order", state.options.meb_proposal_supports);
  j.text("s_comparison_scope", "semantic_labelled_horizontal_forests_not_catalogue_completeness");
  j.boolean("digest_proves_catalogue_completeness", false);
  j.boolean("terminal_root_coverage_proves_equality", false);
  j.number("n", state.options.n); j.number("s", state.options.s);
  j.number("kmax_requested", state.options.kmax); j.number("kmax_effective", state.kmax);
  j.number("completed_orders_diagnostic", state.completed_orders); j.number("last_order", state.order);
  j.text("last_stage", kStageNames[state.stage]);
  j.boolean("frontier_ledger_closed", state.ledger_closed);
  j.boolean("rank_window_regular", state.regular); j.number("rank_relevant_extra_shell", state.extra_shell);
  j.number("raw_candidates", state.raw); j.number("unique_candidates", state.unique);
  j.number("candidate_capacity_observed", state.candidate_capacity);
  j.number("census_balls", state.census_balls); j.number("ball_capacity_observed", state.ball_capacity);
  j.number("generation_cap_refus", state.gen.cap_refus); j.number("emitted_at_refus", state.gen.emitted_at_refus);
  j.number("wave_peak_tasks", state.gen.wave_peak_tasks); j.number("alive_peak_rects", state.gen.alive_peak_rects);
  j.number("pair_mass_expected_per_lane", state.expected_pair_mass);
  if (state.ledger_closed) {
    const char* emitted[3] = {"ledger_q2_emitted", "ledger_q3_emitted", "ledger_q4_emitted"};
    const char* killed[3] = {"ledger_q2_killed", "ledger_q3_killed", "ledger_q4_killed"};
    for (size_t q = 0; q < 3; ++q) {
      j.number(emitted[q], static_cast<u64>(state.gen.ledger_emitted_mass[q]));
      j.number(killed[q], static_cast<u64>(state.gen.ledger_killed_mass[q]));
    }
  }
  j.number("invariant_jneg", state.gen.invariant_jneg);
  j.number("wspd_witness_nodes", state.gen.wspd_witness_nodes);
  j.number("wspd_corner_evals", state.gen.wspd_corner_evals);
  j.number("q4_core_site_tests", state.gen.q4_core_site_tests);
  j.number("q4_completions", state.gen.q4_completions);
  j.number("prefilter_query_nodes", state.expand.depth.nodes);
  j.number("census_query_nodes", state.expand.census.nodes);
  j.number("census_merge_peak_bytes", state.expand.census_merge_peak_bytes);
  j.number("workers_wspd", state.gen.workers_wspd); j.number("workers_rects", state.gen.workers_rects);
  j.number("workers_sort", state.rle_workers); j.number("workers_prefilter", state.expand.workers_prefilter);
  j.number("workers_census", state.expand.workers_census); j.number("workers_expand", state.expand.workers_expand);
  j.key("last_order_work"); std::putchar('{'); std::fputs("\"diagnostic_only\":true", stdout);
  full_work(j, state.last_full); std::putchar('}');
  j.key("stage_ms"); std::putchar('{');
  for (size_t i = 0; i < kStageCount; ++i) {
    if (i != 0) std::putchar(',');
    quoted(kStageNames[i]); std::printf(":%.6f", state.stage_ms[i]);
  }
  std::putchar('}');
  j.real("generation_wspd_ms", state.gen.t_wspd_ms); j.real("generation_rects_ms", state.gen.t_rects_ms);
  j.text("reference_timing", "elapsed_before_terminal_ms_includes_provisional_output");
  j.text("subtracted_timing_scope", "diagnostic_only_not_an_independent_timer");
  j.real("elapsed_before_terminal_ms", wall_ms); j.real("provisional_output_ms", state.output_ms);
  j.real("digest_ms", state.stage_ms[kDigest]);
  j.real("compute_read_release_ms_subtracted_diagnostic", std::max(0.0, wall_ms - state.output_ms));
  j.real("rss_mib_sample", run_detail::rss_mb_now()); j.real("hwm_mib_sample", run_detail::vm_hwm_mb_now());
  j.end();
}
}  // namespace

int main(int argc, char** argv) {
  State state;
  const auto limits = full_probe_limits::make(kPayloadBytes);
  if (argc == 2 && std::strcmp(argv[1], "--digest-selftest") == 0) {
    try {
      const auto r = full_probe_digest::selftest();
      const bool passed = r.checks == 24 && r.failures == 0;
      Json j("digest_selftest"); j.number("checks", r.checks); j.number("failures", r.failures);
      j.number("expected_checks", 24); j.boolean("passed", passed);
      j.text("scope", "bench_serializer_only_no_geometry_or_catalogue_oracle"); j.end();
      return std::fflush(stdout) == 0 && !std::ferror(stdout) ? (passed ? 0 : 4) : 3;
    } catch (...) { return 4; }
  }
  if (argc == 2 && std::strcmp(argv[1], "--help") == 0) {
    std::fputs("full_gabriel_lazy_probe --n=N --s={8,10,12} --kmax={5,10}\n"
               "  --alias-policy=eager | --alias-policy=lazy --cache-entries=C\n"
               "  --meb-proposal-supports={unsigned_decimal|unlimited} (mandatory, shared within each order)\n"
               "  [--threads=N] N positive representable int; explicit option selects schema v6\n"
               "full_gabriel_lazy_probe --digest-selftest\n"
               "Fixed uniform/seed3/coord65536; threads1 by default. FULL builders stay serial.\n"
               "Pipeline worker counts are measured per stage; threads is only the requested budget.\n"
               "N>=2 and C>=0 obey memory/representation limits. No operation quotas.\n"
               "P=0 follows F; finite P exhaustion falls back to F; unlimited retains checked u64 counters.\n"
               "Semantic digests, no archive/vertical export. Monitor larger runs directly.\n", stderr);
    return 2;  // 0 is reserved for a globally completed computation.
  }
  if (!parse_options(argc, argv, state.options, limits)) {
    print_terminal(state, PipelineStatus::kInvalidInput, "probe_arguments", 0);
    return std::fflush(stdout) == 0 && !std::ferror(stdout) ? 2 : 3;
  }
  state.smax = std::min(state.options.kmax + 1, state.options.n);
  state.kmax = std::min(state.options.kmax, state.options.n);
  RunOptions opt;
  opt.s = static_cast<i64>(state.options.s); opt.smax = state.smax; opt.threads = state.options.threads;
  opt.max_raw_candidates = kMaxRawCandidates; opt.memory_budget_bytes = kPayloadBytes;
  FullGabrielLimits caps = limits.full;
  if (state.options.lazy) caps.max_aliases = 0;
  caps.max_meb_proposal_supports = state.options.meb_proposal_supports;
  PipelineStatus status = PipelineStatus::kInvalidInput;
  const char* reason = "probe_uninitialized";
  auto begin = Clock::now();
  try {
    lower_vm_limit(state);
    print_config(state, opt, caps, limits);
    begin = Clock::now();  // Includes generated input, construction, queries and destruction; no archive.
    run(state, opt, caps, limits);
    if (state.completed_orders != state.kmax)
      fail(PipelineStatus::kInvariantViolated, "probe_incomplete_order_loop");
    status = PipelineStatus::kCompleteRegular; reason = kFullGabrielAuthority;
  } catch (const Failure& failure) {
    status = failure.status; reason = failure.reason;
  } catch (const std::bad_alloc&) {
    status = PipelineStatus::kResourceExhausted; reason = "probe_allocation_failed";
  } catch (const std::length_error&) {
    status = PipelineStatus::kResourceExhausted; reason = "probe_size_overflow";
  } catch (const std::system_error& error) {
    const auto code = error.code();
    const bool resource = state.options.threads_set &&
        (code == std::errc::resource_unavailable_try_again || code == std::errc::not_enough_memory ||
         code == std::errc::too_many_files_open || code == std::errc::too_many_files_open_in_system);
    status = resource ? PipelineStatus::kResourceExhausted : PipelineStatus::kInvariantViolated;
    reason = resource ? "probe_thread_resource_unavailable" : "probe_unexpected_exception";
  } catch (const std::exception&) {
    status = PipelineStatus::kInvariantViolated; reason = "probe_unexpected_exception";
  } catch (...) {
    status = PipelineStatus::kInvariantViolated; reason = "probe_unknown_exception";
  }
  print_terminal(state, status, reason, run_detail::ms(begin));
  if (std::fflush(stdout) != 0 || std::ferror(stdout)) return 3;
  return status_exit_code(status);
}
