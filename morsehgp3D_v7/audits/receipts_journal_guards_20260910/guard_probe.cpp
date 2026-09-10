// Causal structural guards and transactional allocation failures only.
// The selected header is the implementation under test, not an oracle.
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <utility>
#include <vector>

#ifndef MHGP7_AUDIT_HEADER
#error MHGP7_AUDIT_HEADER must select the journal header under test
#endif
#include MHGP7_AUDIT_HEADER

namespace allocation_fault {
bool count = false;
size_t calls = 0;
long remaining = -1;

[[gnu::noinline]] void* allocate(size_t n) {
  if (count) ++calls;
  if (remaining == 0) throw std::bad_alloc();
  if (remaining > 0) --remaining;
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}

[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}  // namespace allocation_fault

void* operator new(size_t n) { return allocation_fault::allocate(n); }
void* operator new[](size_t n) { return allocation_fault::allocate(n); }
void operator delete(void* p) noexcept { allocation_fault::release(p); }
void operator delete[](void* p) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t) noexcept { allocation_fault::release(p); }

namespace {
using namespace mhgp7;
using Bank = std::shared_ptr<const FullCoveragePopulations>;
using Batches = std::vector<FullCoverageBatch>;

struct Failure { const char* name; };
struct Counters {
  size_t fixed_checks = 0;
  size_t valid_cases = 0;
  size_t reject_cases = 0;
  size_t reader_checks = 0;
  size_t allocation_calls = 0;
  size_t allocation_rejections = 0;
  int malformed_first_fault_status = -1;
} counters;

void need(bool condition, const char* name) {
  ++counters.fixed_checks;
  if (!condition) throw Failure{name};
}

ExactLevel level(u64 numerator, i128 denominator = 1) {
  return {{numerator, 0, 0}, denominator};
}

Bank make_bank(const std::vector<PointId>& domain,
               const std::vector<FullCoveragePopulation>& rows,
               const char* name) {
  const auto built = build_full_coverage_populations(domain, rows);
  need(built.status == FullCertificateStatus::kOk && built.value, name);
  return built.value;
}

bool empty_unreadable(const FullCoverageCertificate& forest) {
  if (forest.order() || forest.populations() || !forest.nodes().empty() ||
      !forest.parents().empty() || !forest.successors().empty() ||
      !forest.contributions().empty()) return false;
  for (const auto id : {FullNodeId{0}, kFullCoverageAbsent}) {
    if (full_coverage_root_at(forest, id, level(100), true) != kFullCoverageAbsent)
      return false;
  }
  // ABSENT on a valid forest has its own causal reader fixture below.
  const auto read = full_coverage_at(forest, 0, level(100), true);
  return read.status == FullCertificateStatus::kInvalidInput && read.values.empty();
}

FullCoverageCertificate positive(unsigned order, const Bank& bank,
                                 const Batches& batches, const char* name) {
  auto built = build_full_coverage_certificate(order, bank, batches);
  need(built.status == FullCertificateStatus::kOk && built.value.order() == order &&
       built.value.populations() == bank && std::string_view(built.reason) == "structural_only",
       name);
  ++counters.valid_cases;
  return std::move(built.value);
}

void reject(unsigned order, const Bank& bank, const Batches& batches,
            const char* reason, const char* name) {
  const auto built = build_full_coverage_certificate(order, bank, batches);
  need(built.status == FullCertificateStatus::kInvalidInput &&
       std::string_view(built.reason) == reason && empty_unreadable(built.value), name);
  ++counters.reject_cases;
}

Batches one_birth(u16 mask, bool include_interior = false) {
  return {{level(1), {{{}, {{0, mask, include_interior}}}}}};
}

void fixed_guards() {
  {
    const auto good = make_bank({0, 1, 2}, {{{}, {0, 1, 2}}}, "birth_size.good_bank");
    const auto short_row = make_bank({0, 1, 2}, {{{}, {0, 1}}}, "birth_size.short_bank");
    static_cast<void>(positive(3, good, one_birth(7), "birth_size.positive"));
    reject(3, short_row, one_birth(3), "coverage_birth_population", "birth_size.reject");
  }
  {
    const auto bank = make_bank({0, 1, 2}, {{{2}, {0, 1}}}, "birth_interior.bank");
    static_cast<void>(positive(2, bank, one_birth(3, true), "birth_interior.positive"));
    reject(2, bank, one_birth(3, false), "coverage_birth_population", "birth_interior.reject");
  }
  {
    const auto bank = make_bank({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}},
                                "birth_ref_count.bank");
    auto batches = one_birth(3);
    static_cast<void>(positive(2, bank, batches, "birth_ref_count.positive"));
    batches[0].actions[0].contributions.push_back({1, 3, false});
    reject(2, bank, batches, "coverage_birth_population", "birth_ref_count.reject");
  }
  {
    const auto bank = make_bank({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}},
                                "parent_order.bank");
    Batches batches{{level(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}},
                    {level(2), {{{0, 1}, {}}}}};
    const auto forest = positive(2, bank, batches, "parent_order.positive");
    need(forest.parents() == std::vector<FullNodeId>({0, 1}), "parent_order.parent_arena");
    need(forest.successors() == std::vector<FullNodeId>({2, 2, kFullCoverageAbsent}),
         "parent_order.successor_arena");
    need(forest.nodes().size() == 3 && forest.nodes()[0].first == 0 &&
         forest.nodes()[0].parent_count == 0 && forest.nodes()[1].first == 0 &&
         forest.nodes()[1].parent_count == 0 && forest.nodes()[2].first == 0 &&
         forest.nodes()[2].parent_count == 2, "parent_order.csr_bounds");
    batches[1].actions[0].parents = {1, 0};
    reject(2, bank, batches, "coverage_parent_not_unique_prebatch_root", "parent_order.reject");
  }
  {
    const std::vector<PointId> domain{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const auto bank = make_bank(domain, {{{}, domain}}, "order_max.bank");
    static_cast<void>(positive(10, bank, one_birth(2047), "order_max.positive"));
    reject(11, bank, one_birth(2047), "coverage_invalid_domain", "order_max.reject");
  }
  {
    const auto bank = make_bank({0, 1}, {{{}, {0}}, {{}, {1}}}, "k1_count.bank");
    Batches batches{{level(0), {{{}, {{0, 1, false}}}, {{}, {{1, 1, false}}}}}};
    static_cast<void>(positive(1, bank, batches, "k1_count.positive"));
    batches[0].actions.pop_back();
    reject(1, bank, batches, "coverage_k1_roots", "k1_count.reject");
  }
  {
    // Only K changes: the same two-point bank is admissible at n=K=2.
    // This pair checks the exact early reason, separately from birth_size.
    const auto bank = make_bank({0, 1}, {{{}, {0, 1}}}, "domain_below_order.bank");
    static_cast<void>(positive(2, bank, one_birth(3), "domain_below_order.positive"));
    reject(3, bank, one_birth(3), "coverage_invalid_domain", "domain_below_order.reason_only");
  }
}

void read_equals(const FullCoverageCertificate& forest, ExactLevel cut, bool closed,
                 const std::vector<PointId>& expected, const char* name) {
  ++counters.reader_checks;
  const auto read = full_coverage_at(forest, 0, cut, closed);
  need(full_coverage_root_at(forest, 0, cut, closed) == 0 &&
       read.status == FullCertificateStatus::kOk && read.values == expected, name);
}

void absent_rejected(const FullCoverageCertificate& forest, const char* name) {
  ++counters.reader_checks;
  const auto read = full_coverage_at(forest, kFullCoverageAbsent, level(2), true);
  need(read.status == FullCertificateStatus::kInvalidInput && read.values.empty() &&
       full_coverage_root_at(forest, kFullCoverageAbsent, level(2), true) == kFullCoverageAbsent,
       name);
}

void reader_guards() {
  const auto bank = make_bank({0, 1, 2, 3, 4, 5}, {{{}, {0, 1, 2}}, {{5}, {3, 4}}},
                              "reader.bank");
  Batches batches{{level(1), {{{}, {{0, 7, false}}}}},
                  {level(2), {{{0}, {{1, 1, false}}}}}};
  const auto shell_only = positive(3, bank, batches, "reader.shell_only.positive");
  need(shell_only.nodes().size() == 1 && shell_only.parents().empty() &&
       shell_only.successors() == std::vector<FullNodeId>({kFullCoverageAbsent}),
       "reader.continuation_identity");
  read_equals(shell_only, level(2), true, {0, 1, 2, 3}, "reader.shell_only.closed");
  read_equals(shell_only, level(2), false, {0, 1, 2}, "reader.shell_only.open");
  absent_rejected(shell_only, "reader.absent.shell_only");
  batches[1].actions[0].contributions[0].include_interior = true;
  const auto with_interior = positive(3, bank, batches, "reader.with_interior.positive");
  read_equals(with_interior, level(2), true, {0, 1, 2, 3, 5}, "reader.with_interior.closed");
  read_equals(with_interior, level(2), false, {0, 1, 2}, "reader.with_interior.open");
  absent_rejected(with_interior, "reader.absent.with_interior");
}

void allocation_guards() {
  const auto bank = make_bank({0, 1, 2, 3, 4, 5}, {{{}, {0, 1, 2}}, {{}, {3, 4, 5}}},
                              "allocation.bank");
  const Batches batches{{level(1), {{{}, {{0, 7, false}}}, {{}, {{1, 7, false}}}}},
                        {level(2), {{{0}, {{1, 1, false}}}}},
                        {level(3), {{{0, 1}, {}}}}};
  allocation_fault::calls = 0;
  allocation_fault::count = true;
  auto nominal = build_full_coverage_certificate(3, bank, batches);
  allocation_fault::count = false;
  counters.allocation_calls = allocation_fault::calls;
  need(nominal.status == FullCertificateStatus::kOk && nominal.value.nodes().size() == 3 &&
       nominal.value.parents() == std::vector<FullNodeId>({0, 1}) &&
       nominal.value.successors() == std::vector<FullNodeId>({2, 2, kFullCoverageAbsent}),
       "allocation.nominal");
  ++counters.valid_cases;
  need(counters.allocation_calls > 0 &&
       counters.allocation_calls <= static_cast<size_t>(std::numeric_limits<long>::max()),
       "allocation.nonvacuity");
  for (size_t ordinal = 0; ordinal < counters.allocation_calls; ++ordinal) {
    allocation_fault::remaining = static_cast<long>(ordinal);
    auto refused = build_full_coverage_certificate(3, bank, batches);
    allocation_fault::remaining = -1;
    // Keep fixed_checks independent of the allocation strategy under review.
    if (refused.status != FullCertificateStatus::kResourceExhausted ||
        std::string_view(refused.reason) != "coverage_allocation_failed" ||
        !empty_unreadable(refused.value)) throw Failure{"allocation.transactional_reject"};
    ++counters.allocation_rejections;
  }
  need(counters.allocation_rejections == counters.allocation_calls,
       "allocation.all_ordinals_rejected");

  auto invalid_suffix = batches;
  invalid_suffix.back().actions[0].parents = {1, 0};
  reject(3, bank, invalid_suffix, "coverage_parent_not_unique_prebatch_root",
         "malformed.late_reject");

  auto malformed = batches;
  malformed[0].level.den = 0;
  reject(3, bank, malformed, "coverage_invalid_level", "malformed.nominal_reject");
  allocation_fault::remaining = 0;
  auto faulted = build_full_coverage_certificate(3, bank, malformed);
  allocation_fault::remaining = -1;
  counters.malformed_first_fault_status = static_cast<int>(faulted.status);
  need((faulted.status == FullCertificateStatus::kInvalidInput ||
        faulted.status == FullCertificateStatus::kResourceExhausted) &&
       empty_unreadable(faulted.value), "malformed.first_fault_transactional");
}

int run() {
  fixed_guards();
  reader_guards();
  allocation_guards();
  std::printf("{\"fixed_checks\":%zu,\"valid_cases\":%zu,\"reject_cases\":%zu,"
              "\"reader_checks\":%zu,\"allocation_calls\":%zu,"
              "\"allocation_rejections\":%zu,\"malformed_first_fault_status\":%d}\n",
              counters.fixed_checks, counters.valid_cases, counters.reject_cases,
              counters.reader_checks, counters.allocation_calls,
              counters.allocation_rejections, counters.malformed_first_fault_status);
  return 0;
}
}  // namespace

int main(int argc, char**) {
  if (argc != 1) {
    std::fputs("FAIL invalid_arguments\n", stderr);
    return 2;
  }
  try {
    return run();
  } catch (const Failure& failure) {
    allocation_fault::remaining = -1;
    allocation_fault::count = false;
    std::fprintf(stderr, "FAIL %s\n", failure.name);
    return 1;
  } catch (...) {
    allocation_fault::remaining = -1;
    allocation_fault::count = false;
    std::fputs("FAIL unexpected_exception\n", stderr);
    return 1;
  }
}
