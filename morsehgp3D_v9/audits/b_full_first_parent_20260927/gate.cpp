#include "first.hpp"

// Explicit reuse of the frozen batch gate's fixtures and field comparator.
// Replace only its call sites after its encoder header is already guarded.
namespace mhgp9::audit_batch {
inline size_t first_fast_calls = 0, first_fallback_calls = 0;
inline Result first_encode(unsigned k, std::shared_ptr<const FullCoveragePopulations> bank,
                           const FullCoverageFlatDraft& d, bool reverse = false, Mutant mutant = Mutant::None) {
  if (mutant != Mutant::None) return encode(k, std::move(bank), d, reverse, mutant);
  audit_first_parent::Work work;
  auto r = audit_first_parent::encode(k, std::move(bank), d, reverse, audit_first_parent::Mutant::None, &work);
  first_fast_calls += work.fast; first_fallback_calls += work.fallback;
  return r;
}
}  // namespace mhgp9::audit_batch
#define encode first_encode
#include "fixture_helpers.hpp"
#undef encode

namespace {
namespace fp = mhgp9::audit_first_parent;
void fast_histories(Stats& s, const Bank& bank) {
  std::mt19937_64 rng(20260927);
  for (unsigned k = 1; k <= 10; ++k) for (size_t rep = 0; rep < 40; ++rep) {
    auto batches = history(k, bank, rng);
    // Deleting a continuation neither creates nor renumbers a node. Removing
    // the resulting empty batches keeps levels increasing and parents valid.
    for (auto& b : batches) std::erase_if(b.actions, [](const auto& a) { return a.parents.size() == 1; });
    std::erase_if(batches, [](const auto& b) { return b.actions.empty(); });
    need(batches.size() >= 2 && batches[1].actions.front().parents.size() >= 2, "fast.fixture_nonvacuity");
    check(s, k, bank, flat(batches), "structural_only");
    for (unsigned fault = 0; fault < 6; ++fault) {
      auto changed = batches;
      auto& a = changed[1].actions.front();
      switch (fault) {
        case 0: a.parents = {999999, 1000000}; break;
        case 1: a.parents = {0, 0}; break;
        case 2: a.parents = {1, 0}; break;
        case 3: changed[1].actions.push_back(a); break;
        case 4: a.contributions = {ref(999)}; changed[1].actions.push_back(a); break;
        default: a.parents = {999999, 1000000}; changed[1].level = changed[0].level; break;
      }
      check(s, k, bank, flat(changed));
    }
  }
}
void targeted(Stats& s, const Bank& bank) {
  auto b = base();
  b.push_back({level(2), {{{0, 1}, {}}}});
  b.push_back({level(3), {{{0, 2}, {}}}});
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
  b[1].actions.front().contributions = {ref(999)};
  check(s, 2, bank, flat(b), "coverage_population_reference");
  b = base(); b.push_back({level(2), {{{}, {ref(0)}}, {{0, 2}, {}}}});
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
  // Global fallback must be selected even when the continuation occurs late
  // or is itself invalid; no object based on V=A may leak from a prefix.
  b = base(); b.push_back({level(2), {{{0, 1}, {}}}}); b.push_back({level(3), {{{2}, {ref(0)}}}});
  check(s, 2, bank, flat(b), "structural_only");
  b[2].actions[0].contributions.clear(); check(s, 2, bank, flat(b), "coverage_empty_continuation");
  b = base(); b.push_back({level(2), {{{0, 1}, {ref(0, 0), ref(999)}}}});
  check(s, 2, bank, flat(b), "coverage_empty_or_invalid_mask");
  b = base(); b.push_back({level(2), {{{0, 999}, {}}}}); b.push_back({{{0, 0, 0}, 0}, {}});
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
  b = base(); b.push_back({level(2), {{{0, std::numeric_limits<FullNodeId>::max()}, {}}}});
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
  b = base(); b.push_back({level(2), {{{0, 3}, {}}}});
  b.push_back({level(3), {{{}, {ref(0)}}}}); b.push_back({level(4), {{{2, 3}, {}}}});
  check(s, 2, bank, flat(b), "coverage_parent_not_unique_prebatch_root");
}
void first_mutant(const std::string& name, const Bank& bank) {
  auto b = base();
  fp::Mutant mutant = fp::Mutant::None;
  if (name == "ignore-duplicate") {
    mutant = fp::Mutant::IgnoreDuplicate;
    b.push_back({level(2), {{{0, 1}, {}}}}); b.push_back({level(3), {{{0, 2}, {}}}});
  } else if (name == "admit-same-batch") {
    mutant = fp::Mutant::AdmitSameBatch;
    b.push_back({level(2), {{{}, {ref(0)}}, {{0, 2}, {}}}});
  } else if (name == "first-visited") {
    mutant = fp::Mutant::FirstVisitedError;
    b.push_back({level(1), {{{999, 1000}, {ref(999)}}}});
  } else if (name == "last-occurrence") {
    mutant = fp::Mutant::LastOccurrence;
    b.push_back({level(2), {{{0, 1}, {ref(999)}}}}); b.push_back({level(3), {{{0, 2}, {}}}});
  } else throw std::runtime_error("unknown first-parent mutant");
  const auto d = flat(b);
  auto native = build_full_coverage_certificate(2, bank, d);
  fp::Work work;
  auto trial = fp::encode(2, bank, d, mutant != fp::Mutant::LastOccurrence, mutant, &work);
  need(work.fast && !work.fallback, "mutant.fast_path_required");
  if (!equal(native, trial)) {
    std::cout << "{\"schema\":\"mhgp9_full_first_parent_mutant_v1\",\"status\":\"killed\",\"cause\":\"object_or_reason_mismatch\",\"mutant\":\"" << name << "\"}\n";
    std::exit(1);
  }
  throw std::runtime_error("first-parent mutant survived");
}
}  // namespace
int main(int argc, char** argv) {
  try {
    const auto bank = bank_of({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}});
    if (argc == 3 && std::string(argv[1]) == "--mutant") first_mutant(argv[2], bank);
    need(argc == 1, "first-parent usage");
    Stats s; fixtures(s, bank); random_cases(s, large_bank()); targeted(s, bank); fast_histories(s, large_bank());
    need(ab::first_fast_calls > 5000 && ab::first_fallback_calls > 5000 && s.accepted >= 800, "first.nonvacuity");
    std::cout << "{\"schema\":\"mhgp9_full_first_parent_v1\",\"status\":\"passed\",\"cases\":" << s.cases
      << ",\"comparisons\":" << 2*s.cases << ",\"accepted\":" << s.accepted << ",\"rejected\":" << s.rejected
      << ",\"fast_calls\":" << ab::first_fast_calls << ",\"fallback_calls\":" << ab::first_fallback_calls
      << ",\"digest_u64\":" << s.digest << ",\"GCP_used\":false,\"threads_executed\":1}\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
