#include "parallel.hpp"
#include <barrier>
#include <set>

// Reuse frozen fixtures without editing them; route only their encoder calls.
namespace mhgp9::audit_batch {
Result parallel_encode(unsigned, std::shared_ptr<const FullCoveragePopulations>,
                       const FullCoverageFlatDraft&, bool = false, Mutant = Mutant::None);
}
#define encode parallel_encode
#include "../b_full_first_parent_20260927/fixture_helpers.hpp"
#undef encode

namespace {
namespace pp = mhgp9::audit_parallel_parent;
size_t calls = 0, fast_calls = 0, fallback_calls = 0, thread_starts = 0, max_parent_active = 0;
size_t parent_entries = 0;
constexpr std::array<pp::Options, 3> configs{{{1, 1, false}, {4, 1, false}, {4, 7, false}}};
// The much larger historical random gate is qualified separately. These
// fixtures instead exercise each selected history across ALL thread modes.
[[maybe_unused]] const auto inherited_random_cases = &random_cases;

void histories(Stats& s, const Bank& bank) {
  std::mt19937_64 rng(20260927);
  for (unsigned k = 1; k <= 10; ++k) for (unsigned rep = 0; rep < 3; ++rep) {
    auto b = history(k, bank, rng);
    check(s, k, bank, flat(b), "structural_only"); // Global continuation fallback.
    for (auto& batch : b) std::erase_if(batch.actions, [](const auto& a) { return a.parents.size() == 1; });
    std::erase_if(b, [](const auto& batch) { return batch.actions.empty(); });
    need(b.size() >= 2 && b[1].actions.front().parents.size() >= 2, "history.fast");
    check(s, k, bank, flat(b), "structural_only");
    for (unsigned fault = 0; fault < 9; ++fault) {
      auto bad = b; auto& action = bad[1].actions.front();
      switch (fault) {
        case 0: action.parents = {0, 0}; break;
        case 1: action.parents = {0, std::numeric_limits<FullNodeId>::max()}; break;
        case 2: action.contributions = {ref(0, 0), ref(999)}; break;
        case 3: action.contributions = {ref(999)}; bad[1].actions.push_back(action); break;
        case 4: action.parents = {999, 1000}; bad[1].level = bad[0].level; break;
        case 5: action.parents = {999, 1000}; bad.push_back({{{0, 0, 0}, 0}, {}}); break;
        case 6: bad[1].actions.push_back(action); break;
        case 7: bad.back().actions.push_back({{0}, {}}); break; // Late fallback even when invalid.
        default: action.parents = {1, 0}; break;
      }
      check(s, k, bank, flat(bad));
    }
  }
}

void contention(Stats& s, const Bank& bank) {
  std::vector<FullCoverageBatch> b{{level(1), {}}, {level(2), {}}};
  for (size_t a = 0; a < 2048; ++a) b[0].actions.push_back({{}, {ref(a % 2)}});
  for (size_t a = 0; a < 1024; ++a) b[1].actions.push_back({{2*a, 2*a+1}, {}});
  check(s, 2, bank, flat(b), "structural_only");
  // Heavy cross-job contention in the exact same CAS table, but no output
  // scatter is permitted: the earliest bad population precedes all reuses.
  b[1].actions[0].contributions = {ref(999)};
  b.push_back({level(3), {}});
  for (size_t a = 0; a < 1024; ++a) b[2].actions.push_back({{0, 1}, {}});
  check(s, 2, bank, flat(b), "coverage_population_reference");
  b[1].actions[0].contributions.clear();
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
  auto small = base();
  small.push_back({level(2), {{{0, 3}, {}}}});
  small.push_back({level(3), {{{}, {ref(0)}}}});
  small.push_back({level(4), {{{2, 3}, {}}}});
  check(s, 2, bank, flat(small), "coverage_parent_not_unique_prebatch_root");
}

size_t scheduler_gate() {
  std::barrier sync(4);
  std::array<std::thread::id, 4> ids{};
  std::array<unsigned, 4> visits{};
  pp::PhaseStats stat;
  const auto count = pp::parallel_for(4, {4, 1, true}, [&](size_t i, size_t w) {
    ids[w] = std::this_thread::get_id(); ++visits[i]; sync.arrive_and_wait();
  }, {}, &stat);
  need(count == 4 && stat.active == 4 && std::set<std::thread::id>(ids.begin(), ids.end()).size() == 4,
       "scheduler.real_concurrent_threads");
  for (auto x : visits) need(x == 1, "scheduler.exact_partition");
  size_t checks = 1;
  for (size_t grain : {size_t{1}, size_t{7}, std::numeric_limits<size_t>::max()}) {
    std::array<unsigned, 37> hits{};
    pp::parallel_for(hits.size(), {4, grain, true}, [&](size_t i, size_t) { ++hits[i]; });
    for (auto x : hits) need(x == 1, "scheduler.grains"); ++checks;
  }
  // Real launch seam: partial launches must be joined before an exception
  // reaches this scope. The counter is zero immediately after catching.
  for (size_t fail_after : {size_t{0}, size_t{1}, size_t{2}}) {
    std::atomic<unsigned> active{0}; bool caught = false;
    try {
      pp::parallel_for(1024, {4, 1, false}, [&](size_t, size_t) {
        active.fetch_add(1); std::this_thread::yield(); active.fetch_sub(1);
      }, {fail_after});
    } catch (const std::system_error&) { caught = true; }
    need(caught && active.load() == 0, "scheduler.launch_exception_joined"); ++checks;
  }
  for (bool reverse : {false, true}) {
    std::atomic<unsigned> active{0}; bool caught = false;
    try {
      pp::parallel_for(1024, {4, 1, reverse}, [&](size_t i, size_t) {
        active.fetch_add(1); std::this_thread::yield(); active.fetch_sub(1);
        if (i % 4 == 0) throw std::runtime_error("injected worker failure");
      });
    } catch (const std::runtime_error&) { caught = true; }
    need(caught && active.load() == 0, "scheduler.worker_exception_joined"); ++checks;
  }
  need(pp::parallel_for(0, {4, 1, false}, [](size_t, size_t) { throw std::runtime_error("empty job"); }) == 0,
       "scheduler.empty"); ++checks;
  for (auto options : {pp::Options{0, 1, false}, pp::Options{4, 0, false}}) {
    bool caught = false;
    try { pp::parallel_for(1, options, [](size_t, size_t) {}); }
    catch (const std::invalid_argument&) { caught = true; }
    need(caught, "scheduler.invalid_options"); ++checks;
  }
  return checks;
}

void mutant(const std::string& name, const Bank& bank) {
  auto b = base(); pp::Mutant m = pp::Mutant::None;
  if (name == "ignore-duplicate") {
    m = pp::Mutant::IgnoreDuplicate;
    b.push_back({level(2), {{{0, 1}, {}}}}); b.push_back({level(3), {{{0, 2}, {}}}});
  } else if (name == "last-occurrence") {
    m = pp::Mutant::LastOccurrence;
    b.push_back({level(2), {{{0, 1}, {ref(999)}}}}); b.push_back({level(3), {{{0, 2}, {}}}});
  } else if (name == "first-visited") {
    m = pp::Mutant::FirstVisitedError;
    b.push_back({level(1), {{{999, 1000}, {ref(999)}}}});
  } else throw std::runtime_error("unknown mutant");
  const auto d = flat(b);
  const auto native = build_full_coverage_certificate(2, bank, d);
  // Duplicate-admission mutation is deliberately W1: wrong admission must
  // not introduce a deliberate output data race into sanitizer execution.
  const auto trial = pp::encode(2, bank, d, {name == "last-occurrence" ? 4U : 1U, 1, true}, m);
  if (!equal(native, trial)) {
    std::cout << "{\"schema\":\"mhgp9_full_parallel_parent_mutant_v1\",\"status\":\"killed\",\"cause\":\"object_or_reason_mismatch\",\"mutant\":\"" << name << "\"}\n";
    std::exit(1);
  }
  throw std::runtime_error("mutant survived");
}
} // namespace

namespace mhgp9::audit_batch {
Result parallel_encode(unsigned k, std::shared_ptr<const FullCoveragePopulations> bank,
                       const FullCoverageFlatDraft& d, bool reverse, Mutant m) {
  if (m != Mutant::None) return encode(k, std::move(bank), d, reverse, m);
  const auto native = build_full_coverage_certificate(k, bank, d);
  Result out;
  for (auto options : configs) {
    options.reverse = reverse;
    pp::Work work;
    auto trial = pp::encode(k, bank, d, options, pp::Mutant::None, &work);
    need(equal(native, trial), "parallel.object_or_first_reason");
    ++calls; fast_calls += work.fast; fallback_calls += work.fallback;
    thread_starts += work.threads_started; parent_entries += work.parent_entries;
    max_parent_active = std::max(max_parent_active, work.parent_workers_active);
    out = std::move(trial);
  }
  return out;
}
} // namespace

int main(int argc, char** argv) {
  try {
    const auto bank = bank_of({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}});
    if (argc == 3 && std::string(argv[1]) == "--mutant") mutant(argv[2], bank);
    need(argc == 1, "parallel usage");
    const auto scheduler_checks = scheduler_gate();
    Stats stats; fixtures(stats, bank); histories(stats, large_bank()); contention(stats, bank);
    need(calls == 6 * stats.cases && max_parent_active == 4 && fast_calls > 1000 && fallback_calls > 100,
         "parallel.nonvacuity");
    alignas(std::atomic_ref<u64>::required_alignment) u64 sample = 0;
    std::cout << "{\"schema\":\"mhgp9_full_parallel_parent_v1\",\"status\":\"passed\",\"cases\":" << stats.cases
      << ",\"comparisons\":" << calls << ",\"accepted\":" << stats.accepted << ",\"rejected\":" << stats.rejected
      << ",\"fast_calls\":" << fast_calls << ",\"fallback_calls\":" << fallback_calls
      << ",\"digest_u64\":" << stats.digest << ",\"threads_started\":" << thread_starts
      << ",\"parent_occurrences_checked\":" << parent_entries << ",\"parent_workers_active\":" << max_parent_active
      << ",\"scheduler_checks\":" << scheduler_checks << ",\"simultaneous_threads\":4,\"workers\":[1,4],\"grains\":[1,7],\"schedules\":2"
      << ",\"atomic_ref_size\":" << sizeof(std::atomic_ref<u64>) << ",\"atomic_cell_size\":" << sizeof(u64)
      << ",\"atomic_required_alignment\":" << std::atomic_ref<u64>::required_alignment
      << ",\"atomic_is_lock_free\":" << (std::atomic_ref<u64>(sample).is_lock_free() ? "true" : "false")
      << ",\"GCP_used\":false}\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
