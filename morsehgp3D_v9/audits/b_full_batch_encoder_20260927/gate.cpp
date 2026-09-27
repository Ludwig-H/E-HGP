#include "encode.hpp"

#include <iostream>
#include <numeric>
#include <random>
#include <string>

namespace {
using namespace mhgp9::tower;
namespace ab = mhgp9::audit_batch;
using Bank = std::shared_ptr<const FullCoveragePopulations>;
void need(bool p, const char* reason) { if (!p) throw std::runtime_error(reason); }
ExactLevel level(u64 n, u64 scale = 2) { return {{n * scale, 0, 0}, static_cast<i128>(scale)}; }
FullCoverageRef ref(u64 p, u16 mask = 3, bool inside = false) { return {p, mask, inside}; }
FullCoverageFlatDraft flat(const std::vector<FullCoverageBatch>& b) {
  FullCoverageFlatDraft d;
  for (const auto& x : b) { d.open_batch(x.level); for (const auto& a : x.actions) d.add_action(a.parents, a.contributions); }
  return d;
}
Bank bank_of(const std::vector<PointId>& domain, const std::vector<FullCoveragePopulation>& rows) {
  auto r = build_full_coverage_populations(domain, std::span<const FullCoveragePopulation>(rows));
  need(r.status == FullCertificateStatus::kOk, "fixture.bank"); return r.value;
}
bool same_ref(const FullCoverageRef& a, const FullCoverageRef& b) {
  return a.population == b.population && a.shell_mask == b.shell_mask && a.include_interior == b.include_interior;
}
bool equal(const FullCoverageBuildResult& a, const ab::Result& b) {
  if (a.status != b.status || std::string(a.reason) != b.reason || a.value.order() != b.order ||
      a.value.populations() != b.bank || a.value.nodes().size() != b.nodes.size() ||
      a.value.parents() != b.parents || a.value.successors() != b.successors ||
      a.value.contributions().size() != b.contributions.size()) return false;
  for (size_t i = 0; i < b.nodes.size(); ++i) {
    const auto& x = a.value.nodes()[i]; const auto& y = b.nodes[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  for (size_t i = 0; i < b.contributions.size(); ++i) {
    const auto& x = a.value.contributions()[i]; const auto& y = b.contributions[i];
    if (x.level != y.level || x.segment != y.segment || !same_ref(x.ref, y.ref)) return false;
  }
  return true;
}
struct Stats {
  size_t cases = 0, accepted = 0, rejected = 0, nodes = 0, parents = 0, contributions = 0;
  std::array<size_t, 11> orders{};
  u64 digest = 1469598103934665603ULL;
  void word(u64 w) { digest = (digest ^ w) * 1099511628211ULL; }
};
void check(Stats& s, unsigned k, const Bank& bank, const FullCoverageFlatDraft& d,
           const char* expected = nullptr) {
  auto native = build_full_coverage_certificate(k, bank, d);
  if (expected) need(std::string(native.reason) == expected, "fixture.expected_reason");
  for (bool reverse : {false, true}) {
    const auto trial = ab::encode(k, bank, d, reverse);
    if (!equal(native, trial)) {
      std::cerr << "case=" << s.cases << " k=" << k << " reverse=" << reverse
                << " native=" << native.reason << " trial=" << trial.reason << '\n';
      throw std::runtime_error("batch.differential_mismatch");
    }
  }
  ++s.cases; if (k <= 10) ++s.orders[k];
  s.word(k); s.word(static_cast<u64>(native.status));
  for (const char* p = native.reason; *p; ++p) s.word(static_cast<unsigned char>(*p));
  if (native.status == FullCertificateStatus::kOk) {
    ++s.accepted; s.nodes += native.value.nodes().size(); s.parents += native.value.parents().size();
    s.contributions += native.value.contributions().size();
    for (const auto& n : native.value.nodes()) { for (auto w : n.level.num) s.word(w); s.word(static_cast<u64>(n.level.den)); s.word(n.first); s.word(n.parent_count); }
    for (auto p : native.value.parents()) s.word(p);
    for (auto p : native.value.successors()) s.word(p);
    for (const auto& c : native.value.contributions()) { s.word(c.segment); s.word(c.ref.population); s.word(c.ref.shell_mask); s.word(c.ref.include_interior); }
  } else {
    ++s.rejected;
    need(native.value.order() == 0 && !native.value.populations() && native.value.nodes().empty() &&
         native.value.parents().empty() && native.value.successors().empty() && native.value.contributions().empty(),
         "fixture.native_transactional");
  }
}
std::vector<FullCoverageBatch> base() {
  return {{level(1), {{{}, {ref(0)}}, {{}, {ref(1)}}}}};
}
void fixtures(Stats& s, const Bank& bank) {
  const auto test = [&](std::vector<FullCoverageBatch> b, const char* reason) { check(s, 2, bank, flat(b), reason); };
  test(base(), "structural_only");
  auto b = base(); b.push_back({level(2), {{{0, 1}, {}}}}); b.push_back({level(3), {{{0}, {ref(0, 1)}}}});
  test(b, "coverage_parent_not_unique_prebatch_root");
  b[1].actions[0].contributions = {ref(999)}; test(b, "coverage_population_reference");
  b = base(); b.push_back({level(2), {{{0}, {ref(0, 1)}}, {{0, 1}, {}}}});
  test(b, "coverage_parent_not_unique_prebatch_root");
  b[1].actions[0].contributions.clear(); test(b, "coverage_empty_continuation");
  b = base(); b.push_back({level(2), {{{}, {ref(0)}}, {{2}, {ref(0, 1)}}}});
  test(b, "coverage_parent_not_unique_prebatch_root");
  b = base(); b.push_back({level(2), {{{999}, {ref(0, 1)}}}}); b.push_back({{{0, 0, 0}, 0}, {}});
  test(b, "coverage_parent_not_unique_prebatch_root");
  b[1].level = level(1); test(b, "coverage_nonincreasing_batch");
  b = base(); b.push_back({level(2), {{{0}, {ref(0, 0), ref(999)}}}});
  test(b, "coverage_empty_or_invalid_mask");
  b = base(); b.push_back({level(2), {{{}, {ref(0), ref(1)}}}});
  test(b, "coverage_birth_population"); b[1].actions[0].contributions[1] = ref(999);
  test(b, "coverage_population_reference");
  auto d = flat(base()); ++d.contribution_begin.back(); check(s, 0, nullptr, d, "coverage_flat_draft_shape");
  d = flat(base()); check(s, 0, bank, d, "coverage_invalid_domain"); check(s, 11, bank, d, "coverage_invalid_domain");
  check(s, 2, nullptr, d, "coverage_invalid_domain");
  auto empty_bank = std::make_shared<FullCoveragePopulations>(); check(s, 2, empty_bank, d, "coverage_invalid_domain");
  check(s, 2, bank, {}, "coverage_invalid_domain");
  b = base(); b[0].level.den = -1; test(b, "coverage_invalid_level");
  b = base(); b[0].level = level(0); test(b, "coverage_positive_level_required");
  b = base(); b.push_back({level(2), {}}); test(b, "coverage_empty_batch");
  b = base(); b.push_back({level(2), {{{1, 0}, {}}}}); test(b, "coverage_parent_not_unique_prebatch_root");
  b = base(); b.push_back({level(2), {{{0, 0}, {}}}}); test(b, "coverage_parent_not_unique_prebatch_root");
  b = base(); b[0].actions[0].contributions = {ref(0, 1)}; test(b, "coverage_birth_population");
  b = base(); b[0].actions[0].contributions = {ref(0, 4)}; test(b, "coverage_empty_or_invalid_mask");
  b = base(); b[0].actions[0].contributions = {ref(0, 3, true)}; test(b, "coverage_empty_or_invalid_mask");
  // A silent multifusion is legal. A continuation contributes and creates no node.
  b = base(); b.push_back({level(2), {{{0}, {ref(0, 1)}}, {{1}, {ref(1, 1)}}}});
  b.push_back({level(3), {{{0, 1}, {}}}}); test(b, "structural_only");
  // Preserve all three numerator words; equal rational representations form
  // one plateau and must not become successive batches.
  b = base(); b[0].level = {{0, 0, 1}, 1}; test(b, "structural_only");
  b.push_back({{{0, 0, 2}, 2}, {{{0}, {ref(0, 1)}}}}); test(b, "coverage_nonincreasing_batch");
  // More than 13 parents (32), no binary-fold surrogate.
  b = {{level(1), {}}}; for (size_t i = 0; i < 32; ++i) b[0].actions.push_back({{}, {ref(i % 2)}});
  std::vector<FullNodeId> parents(32); std::iota(parents.begin(), parents.end(), 0);
  b.push_back({level(2), {{parents, {ref(0), ref(1)}}}}); test(b, "structural_only");
  // Public flat boundary is checked before any offset-based read.
  for (unsigned fault = 0; fault < 9; ++fault) {
    d = flat(base());
    switch (fault) {
      case 0: d.parent_begin.clear(); break;
      case 1: d.batch_begin.clear(); break;
      case 2: d.contribution_begin.clear(); break;
      case 3: d.batch_begin[0] = 1; break;
      case 4: d.parent_begin[0] = 1; break;
      case 5: d.contribution_begin[0] = 1; break;
      case 6: d.batch_begin.back() = std::numeric_limits<u64>::max(); break;
      case 7: d.parent_begin[1] = 1; break;
      default: d.contribution_begin[1] = 9; break;
    }
    check(s, 2, bank, d, "coverage_flat_draft_shape");
  }
}

Bank large_bank() {
  std::vector<PointId> domain(64); std::iota(domain.begin(), domain.end(), 0);
  std::vector<FullCoveragePopulation> rows;
  for (PointId id : domain) rows.push_back({{}, {id}});
  for (unsigned k = 1; k <= 10; ++k) {
    FullCoveragePopulation row;
    for (PointId id = 0; id < k; ++id) (id < k / 2 ? row.interior : row.shell).push_back(id);
    rows.push_back(std::move(row));
  }
  // Exactly sixteen shell entries exercises the u16 full mask without UB.
  rows.push_back({{}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}});
  return bank_of(domain, rows);
}
FullCoverageRef complete(const Bank& bank, size_t p) {
  const auto& row = bank->rows()[p]; return {p, ab::shell_mask(row), !row.interior.empty()};
}
std::vector<FullCoverageBatch> history(unsigned k, const Bank& bank, std::mt19937_64& rng) {
  std::vector<FullCoverageBatch> batches{{level(k == 1 ? 0 : 1), {}}};
  std::vector<FullNodeId> live;
  FullNodeId next = 0;
  const size_t births = k == 1 ? bank->domain().size() : 4 + rng() % 20;
  for (size_t i = 0; i < births; ++i) {
    batches[0].actions.push_back({{}, {complete(bank, k == 1 ? i : 63 + k)}}); live.push_back(next++);
  }
  for (size_t b = 1; b < 9; ++b) {
    FullCoverageBatch batch{level(b + 1), {}};
    std::shuffle(live.begin(), live.end(), rng);
    std::vector<FullNodeId> updated;
    for (size_t i = 0; i < live.size();) {
      const size_t n = std::min<size_t>(1 + rng() % 4, live.size() - i);
      std::vector<FullNodeId> ps(live.begin() + static_cast<std::ptrdiff_t>(i), live.begin() + static_cast<std::ptrdiff_t>(i + n));
      std::sort(ps.begin(), ps.end());
      std::vector<FullCoverageRef> cs;
      if (n == 1 || rng() % 2) cs.push_back(complete(bank, 63 + k));
      if (rng() % 4 == 0) cs.push_back(complete(bank, 74));
      batch.actions.push_back({ps, cs}); updated.push_back(n == 1 ? ps.front() : next++); i += n;
    }
    if (k > 1 && rng() % 2) { batch.actions.push_back({{}, {complete(bank, 63 + k)}}); updated.push_back(next++); }
    batches.push_back(std::move(batch)); live = std::move(updated);
  }
  return batches;
}
void random_cases(Stats& s, const Bank& bank) {
  std::mt19937_64 rng(20260927);
  for (unsigned k = 1; k <= 10; ++k) for (size_t rep = 0; rep < 40; ++rep) {
    const auto base_history = history(k, bank, rng);
    check(s, k, bank, flat(base_history), "structural_only");
    for (unsigned fault = 0; fault < 16; ++fault) {
      auto b = base_history;
      auto& a = b[1].actions[0];
      switch (fault) {
        case 0: b[1].level.den = 0; break;
        case 1: b[1].level = b[0].level; break;
        case 2: b[1].actions.clear(); break;
        case 3: a.parents = {std::numeric_limits<FullNodeId>::max()}; break;
        case 4: a.parents = {0, 0}; break;
        case 5: a.parents = {1, 0}; break;
        case 6: a.parents = {0}; a.contributions.clear(); break;
        case 7: a.contributions = {ref(999)}; break;
        case 8: a.contributions = {ref(0, 2)}; break;
        case 9: a.contributions = {ref(0, 0)}; break;
        case 10: a.parents.clear(); a.contributions.clear(); break;
        case 11: a.parents.clear(); a.contributions = {complete(bank, 0), complete(bank, 1)}; break;
        case 12: a.parents.clear(); a.contributions = {complete(bank, 0)}; break;
        case 13: a.parents = {0}; a.contributions = {ref(0, 0), ref(999)}; break;
        case 14: a.contributions = {ref(999)}; b[2].level.den = 0; break;
        default: a.parents = {999}; b[1].level = b[0].level; break;
      }
      check(s, k, bank, flat(b));
    }
  }
}
void mutant_case(const std::string& name, const Bank& bank) {
  auto b = base(); ab::Mutant mutant = ab::Mutant::None;
  if (name == "ignore-dead") {
    mutant = ab::Mutant::IgnoreDead;
    b.push_back({level(2), {{{0, 1}, {}}}}); b.push_back({level(3), {{{0}, {ref(0, 1)}}}});
  } else if (name == "first-visited") {
    mutant = ab::Mutant::FirstVisitedError;
    b.push_back({level(1), {{{999}, {ref(999)}}}});
  } else if (name == "normalize-level") mutant = ab::Mutant::NormalizeLevel;
  else throw std::runtime_error("unknown mutant");
  const auto d = flat(b);
  const auto native = build_full_coverage_certificate(2, bank, d);
  const auto trial = ab::encode(2, bank, d, true, mutant);
  // Same executable, deliberate semantic mutation. Exit 1 means a detected
  // object/reason mismatch, not a crash or a sanitizer error.
  if (!equal(native, trial)) { std::cout << "{\"schema\":\"mhgp9_full_batch_mutant_v1\",\"mutant\":\"" << name << "\",\"status\":\"killed\",\"cause\":\"object_or_reason_mismatch\"}\n"; std::exit(1); }
  throw std::runtime_error("mutant survived");
}
}  // namespace

int main(int argc, char** argv) {
  try {
    const auto small = bank_of({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}});
    if (argc == 3 && std::string(argv[1]) == "--mutant") mutant_case(argv[2], small);
    need(argc == 1, "usage");
    Stats s; fixtures(s, small); random_cases(s, large_bank());
    need(s.accepted >= 400 && s.rejected >= 6000 && s.nodes > 10000, "gate.nonvacuity");
    for (unsigned k = 1; k <= 10; ++k) need(s.orders[k] >= 680, "gate.order_coverage");
    std::cout << "{\"schema\":\"mhgp9_full_batch_encoder_v1\",\"status\":\"passed\",\"scope\":\"structural_flat_only\",\"cases\":" << s.cases
              << ",\"schedules\":2,\"comparisons\":" << 2 * s.cases << ",\"accepted\":" << s.accepted
              << ",\"rejected\":" << s.rejected << ",\"nodes\":" << s.nodes << ",\"parents\":" << s.parents
              << ",\"contributions\":" << s.contributions << ",\"digest_u64\":" << s.digest
              << ",\"seed\":20260927,\"GCP_used\":false,\"threads_executed\":1}\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
