// Independent structural_mixed expectations from receipts_coverage_cpp_20260910/corpus.py.
// Each prefix is built and sealed by a fresh owner; no private-state access.
// The same-header facade comparison is supplementary, NOT an independent oracle.
#include <array>
#include <cstdio>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#ifndef MHGP7_AUDIT_HEADER
#error "MHGP7_AUDIT_HEADER must name the pinned full_coverage_incremental.hpp"
#endif
#include MHGP7_AUDIT_HEADER

namespace {
using namespace mhgp7;
using namespace mhgp7::full_coverage_detail;
constexpr FullNodeId absent = std::numeric_limits<FullNodeId>::max();
constexpr std::array<size_t, 6> node_counts{0, 4, 6, 6, 7, 7};
constexpr std::array<size_t, 6> parent_counts{0, 0, 3, 3, 6, 6};
constexpr std::array<size_t, 6> contribution_counts{0, 4, 8, 9, 9, 11};
constexpr std::array<unsigned, 7> node_dates{1, 1, 1, 1, 2, 2, 4};
constexpr std::array<FullNodeId, 6> parent_ids{0, 1, 2, 3, 4, 5};
// Rows are complete states after each lot. Point masks apply only to live roots.
constexpr FullNodeId expected_roots[6][7] = {
    {absent, absent, absent, absent, absent, absent, absent},
    {0, 1, 2, 3, absent, absent, absent},
    {5, 5, 5, 3, 4, 5, absent},
    {5, 5, 5, 3, 4, 5, absent},
    {6, 6, 6, 6, 6, 6, 6},
    {6, 6, 6, 6, 6, 6, 6}};
constexpr unsigned expected_points[6][7] = {
    {0, 0, 0, 0, 0, 0, 0},
    {3, 6, 24, 48, 0, 0, 0},
    {0, 0, 0, 304, 192, 543, 0},
    {0, 0, 0, 304, 193, 543, 0},
    {0, 0, 0, 0, 0, 0, 1023},
    {0, 0, 0, 0, 0, 0, 1023}};
constexpr unsigned contribution_dates[11] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 5, 5};
constexpr FullNodeId contribution_segments[11] = {0, 1, 2, 3, 4, 3, 3, 5, 4, 6, 6};
constexpr u64 contribution_populations[11] = {0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 5};
constexpr u16 contribution_masks[11] = {1, 3, 3, 3, 1, 256, 256, 512, 1, 1023, 256};
constexpr bool contribution_interiors[11] = {true, false, false, false, true,
                                            false, false, false, false, false, false};
struct Failure { const char* reason; };
size_t checks = 0, cut_queries = 0, invariant_queries = 0, successor_writes = 0;
size_t closed_changes = 0, rejected_suffixes = 0, facade_comparisons = 0;
void need(bool condition, const char* reason) {
  ++checks;
  if (!condition) throw Failure{reason};
}
bool same_level(const ExactLevel& a, const ExactLevel& b) {
  return a.num[0] == b.num[0] && a.num[1] == b.num[1] &&
      a.num[2] == b.num[2] && a.den == b.den;
}
bool same_node(const FullNode& a, const FullNode& b) {
  return same_level(a.level, b.level) && a.first == b.first && a.parent_count == b.parent_count;
}
bool same_contribution(const FullDatedContribution& a, const FullDatedContribution& b) {
  return same_level(a.level, b.level) && a.segment == b.segment &&
      a.ref.population == b.ref.population && a.ref.shell_mask == b.ref.shell_mask &&
      a.ref.include_interior == b.ref.include_interior;
}

struct Fixture {
  bool wide;
  unsigned factor;
  std::vector<PointId> domain{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<FullCoveragePopulation> rows{
      {{0}, {1}}, {{}, {1, 2}}, {{}, {3, 4}}, {{}, {4, 5}}, {{6}, {7}},
      {{}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}}};
  std::vector<FullCoverageBatch> batches;
  i128 denominator() const { return wide ? (i128{1} << 120) - 1 : 1; }
  ExactLevel at(unsigned date) const {
    return {{static_cast<u64>(date) * factor, 0, wide ? u64{factor} << 52 : 0},
            denominator() * factor};
  }
  // Independent rational grid (base + twice/2)/D. Factor 3 deliberately differs
  // from both journal encodings; expected admission uses integers, not C++ levels.
  ExactLevel cut(unsigned twice) const {
    return {{static_cast<u64>(twice) * 3, 0, wide ? u64{3} << 53 : 0},
            denominator() * 6};
  }
  Fixture(bool is_wide, unsigned multiplier) : wide(is_wide), factor(multiplier) {
    batches = {
        {at(1), {{{}, {{0, 1, true}}}, {{}, {{1, 3, false}}},
                  {{}, {{2, 3, false}}}, {{}, {{3, 3, false}}}}},
        {at(2), {{{}, {{4, 1, true}}}, {{3}, {{5, 256, false}, {5, 256, false}}},
                  {{0, 1, 2}, {{5, 512, false}}}}},
        {at(3), {{{4}, {{5, 1, false}}}}},
        {at(4), {{{3, 4, 5}, {}}}},
        {at(5), {{{6}, {{5, 1023, false}, {5, 256, false}}}}}};
  }
};

void populate(CoverageTowerAssembler& owner, const Fixture& f) {
  for (size_t i = 0; i < f.rows.size(); ++i) {
    const auto row = owner.append_population(f.rows[i]);
    need(row.status == FullCertificateStatus::kOk && row.population == i, "population.append");
  }
}
void append_prefix(CoverageTowerAssembler& owner, const CoverageOrderToken& token,
    const Fixture& f, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    CoverageAppendResult result;
    if (f.factor == 1) {
      result = owner.append_batch(token, f.batches[i]);
    } else {
      std::vector<CoverageActionView> views;
      for (const auto& action : f.batches[i].actions)
        views.push_back({action.parents, action.contributions});
      result = owner.append_batch(token, f.batches[i].level,
                                 std::span<const CoverageActionView>(views));
    }
    need(result.status == FullCertificateStatus::kOk, "append.status");
    need(result.first_new_node == node_counts[i] &&
        result.new_node_count == node_counts[i + 1] - node_counts[i], "append.node.ids");
  }
}
FullCoverageCertificate build_prefix(const Fixture& f, size_t count) {
  CoverageTowerAssembler owner(f.domain);
  populate(owner, f);
  const auto order = owner.begin_order(2);
  need(order.status == FullCertificateStatus::kOk, "order.begin");
  append_prefix(owner, order.token, f, count);
  need(owner.end_order(order.token).status == FullCertificateStatus::kOk, "order.end");
  auto result = owner.seal();
  need(result.status == FullCertificateStatus::kOk && result.orders.size() == 1 &&
      result.populations && result.orders[0].populations() == result.populations, "prefix.seal");
  return std::move(result.orders[0]);
}

void physical(const Fixture& f, const FullCoverageCertificate& forest, size_t prefix) {
  need(forest.order() == 2 && forest.nodes().size() == node_counts[prefix], "node.count");
  need(forest.parents().size() == parent_counts[prefix] &&
      forest.successors().size() == node_counts[prefix] &&
      forest.contributions().size() == contribution_counts[prefix], "arena.sizes");
  need(forest.populations()->domain() == f.domain &&
      forest.populations()->rows().size() == f.rows.size(), "bank.shape");
  for (size_t i = 0; i < f.rows.size(); ++i)
    need(forest.populations()->rows()[i].interior == f.rows[i].interior &&
        forest.populations()->rows()[i].shell == f.rows[i].shell, "bank.values");
  for (size_t i = 0; i < forest.nodes().size(); ++i) {
    const auto& node = forest.nodes()[i];
    need(same_level(node.level, f.at(node_dates[i])), "node.level");
    need(node.first == (i == 6 ? 3U : 0U) && node.parent_count == (i >= 5 ? 3U : 0U),
         "node.parents.csr");
    FullNodeId next = absent;
    if (prefix >= 2 && i < 3) next = 5;
    if (prefix >= 4 && i >= 3 && i <= 5) next = 6;
    need(forest.successors()[i] == next, "successor.expected");
  }
  for (size_t i = 0; i < forest.parents().size(); ++i)
    need(forest.parents()[i] == parent_ids[i], "parent.expected");
  for (size_t i = 0; i < forest.contributions().size(); ++i) {
    const auto& contribution = forest.contributions()[i];
    need(same_level(contribution.level, f.at(contribution_dates[i])), "contribution.level");
    need(contribution.segment == contribution_segments[i] &&
        contribution.ref.population == contribution_populations[i] &&
        contribution.ref.shell_mask == contribution_masks[i] &&
        contribution.ref.include_interior == contribution_interiors[i], "contribution.expected");
  }
  // This facade shares the candidate's kernel; only the fixed tables above and
  // below supply independent expectations. It checks the additional API route.
  const auto bank = build_full_coverage_populations(f.domain, f.rows);
  need(bank.status == FullCertificateStatus::kOk, "facade.bank");
  auto facade = build_full_coverage_certificate(2, bank.value,
      std::span<const FullCoverageBatch>(f.batches.data(), prefix));
  need(facade.status == FullCertificateStatus::kOk, "facade.status");
  need(facade.value.nodes().size() == forest.nodes().size() &&
      facade.value.parents() == forest.parents() && facade.value.successors() == forest.successors() &&
      facade.value.contributions().size() == forest.contributions().size(), "facade.arenas");
  for (size_t i = 0; i < forest.nodes().size(); ++i)
    need(same_node(facade.value.nodes()[i], forest.nodes()[i]), "facade.node");
  for (size_t i = 0; i < forest.contributions().size(); ++i)
    need(same_contribution(facade.value.contributions()[i], forest.contributions()[i]), "facade.contribution");
  ++facade_comparisons;
}

FullNodeId query_id(size_t index) { return index == 8 ? absent : index; }
void check_cut(const Fixture& f, const FullCoverageCertificate& forest, size_t prefix,
    unsigned twice, bool closed) {
  size_t stage = 0;
  for (size_t i = 1; i <= prefix; ++i)
    if (2 * i < twice || (2 * i == twice && closed)) stage = i;
  const auto cut = f.cut(twice);
  for (size_t j = 0; j < 9; ++j) {
    const auto id = query_id(j);
    const auto root = j < 7 ? expected_roots[stage][j] : absent;
    need(full_coverage_root_at(forest, id, cut, closed) == root, "cut.root.expected");
    const auto read = full_coverage_at(forest, id, cut, closed);
    if (root != absent && root == id) {
      std::vector<PointId> points;
      for (PointId point = 0; point < 10; ++point)
        if (expected_points[stage][j] & (1U << point)) points.push_back(point);
      need(read.status == FullCertificateStatus::kOk && read.values == points, "cut.coverage.expected");
    } else {
      need(read.status == FullCertificateStatus::kInvalidInput && read.values.empty(), "cut.absent.expected");
    }
    ++cut_queries;
  }
}
bool equal_cut(const FullCoverageCertificate& a, const FullCoverageCertificate& b,
    const ExactLevel& cut, bool closed) {
  bool same = true;
  for (size_t j = 0; j < 9; ++j) {
    const auto id = query_id(j);
    const auto left = full_coverage_at(a, id, cut, closed);
    const auto right = full_coverage_at(b, id, cut, closed);
    if (full_coverage_root_at(a, id, cut, closed) != full_coverage_root_at(b, id, cut, closed) ||
        left.status != right.status || left.values != right.values) same = false;
  }
  return same;
}
void transition(const Fixture& f, const FullCoverageCertificate& before,
    const FullCoverageCertificate& after, size_t prefix) {
  for (size_t i = 0; i < before.nodes().size(); ++i) {
    need(same_node(before.nodes()[i], after.nodes()[i]), "prefix.node.immutable");
    const auto old_next = before.successors()[i], new_next = after.successors()[i];
    if (old_next != absent) need(old_next == new_next, "successor.preexisting.immutable");
    if (old_next != new_next) {
      need(old_next == absent && new_next >= before.nodes().size() &&
          new_next < after.nodes().size(), "successor.single.assignment");
      need(after.nodes()[new_next].parent_count >= 2 &&
          same_level(after.nodes()[new_next].level, f.at(static_cast<unsigned>(prefix))),
          "successor.current.fusion");
      ++successor_writes;
    }
  }
  for (size_t i = 0; i < before.parents().size(); ++i)
    need(before.parents()[i] == after.parents()[i], "prefix.parent.immutable");
  for (size_t i = 0; i < before.contributions().size(); ++i)
    need(same_contribution(before.contributions()[i], after.contributions()[i]), "prefix.contribution.immutable");
  for (unsigned twice = 0; twice <= 2 * prefix; ++twice) {
    for (bool closed : {false, true}) {
      if (twice == 2 * prefix && closed) continue;
      need(equal_cut(before, after, f.cut(twice), closed), "prefix.old.cut.immutable");
      ++invariant_queries;
    }
  }
  const bool unchanged = equal_cut(before, after, f.cut(static_cast<unsigned>(2 * prefix)), true);
  // Lots 1..4 change a root or its coverage. Lot 5 intentionally adds only
  // redundant contributions; its closed coverage must remain unchanged.
  need(unchanged == (prefix == 5), "prefix.closed.boundary");
  if (!unchanged) ++closed_changes;
}

void late_rejections(const Fixture& f) {
  for (unsigned variant = 0; variant < 4; ++variant) {
    CoverageTowerAssembler owner(f.domain);
    populate(owner, f);
    const auto finished = owner.begin_order(2);
    need(finished.status == FullCertificateStatus::kOk, "rejection.first.order");
    append_prefix(owner, finished.token, f, 5);
    need(owner.end_order(finished.token).status == FullCertificateStatus::kOk, "rejection.first.closed");
    const auto active = owner.begin_order(2);
    need(active.status == FullCertificateStatus::kOk, "rejection.next.order");
    append_prefix(owner, active.token, f, 2);
    FullCoverageBatch bad{f.at(3), {{{0, 1}, {}}}};
    const char* reason = "coverage_parent_not_unique_prebatch_root";
    if (variant == 1) bad.actions = {{{3}, {{5, 1, false}}}, {{3, 4}, {}}};
    if (variant == 2) bad.actions = {{{}, {{4, 1, true}}}, {{3, 6}, {}}};
    if (variant == 3) {
      bad = f.batches[2]; bad.level = f.at(2); reason = "coverage_nonincreasing_batch";
    }
    const auto rejected = owner.append_batch(active.token, bad);
    need(rejected.status == FullCertificateStatus::kInvalidInput &&
        std::string_view(rejected.reason) == reason, "late.parent.or.level.rejection");
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
      const auto result = owner.seal();
      need(result.status == FullCertificateStatus::kInvalidInput &&
          std::string_view(result.reason) == reason && !result.populations && result.orders.empty(),
          "late.failure.global.empty");
      need(owner.append_batch(active.token, f.batches[2]).status == FullCertificateStatus::kInvalidInput &&
          owner.append_population(f.rows[0]).status == FullCertificateStatus::kInvalidInput &&
          owner.end_order(active.token).status == FullCertificateStatus::kInvalidInput,
          "late.failure.cannot.resume");
    }
    ++rejected_suffixes;
  }
}

int run() {
  for (bool wide : {false, true}) for (unsigned factor : {1U, 2U}) {
    const Fixture fixture(wide, factor);
    need(fixture.batches[1].actions.size() == 3 &&
        fixture.batches[1].actions[0].parents.empty() &&
        fixture.batches[1].actions[1].parents.size() == 1 &&
        fixture.batches[1].actions[2].parents.size() == 3, "fixture.mixed.nonvacuity");
    std::vector<FullCoverageCertificate> prefixes;
    prefixes.reserve(6);
    prefixes.emplace_back();  // Empty reader state only; no empty prefix is published.
    for (size_t prefix = 1; prefix <= 5; ++prefix) {
      prefixes.push_back(build_prefix(fixture, prefix));
      physical(fixture, prefixes.back(), prefix);
      transition(fixture, prefixes[prefix - 1], prefixes[prefix], prefix);
    }
    for (size_t prefix = 0; prefix <= 5; ++prefix)
      for (unsigned twice = 0; twice <= 12; ++twice)
        for (bool closed : {false, true}) check_cut(fixture, prefixes[prefix], prefix, twice, closed);
    late_rejections(fixture);
  }
  need(cut_queries == 5616 && invariant_queries == 260 && successor_writes == 24 &&
      closed_changes == 16 && rejected_suffixes == 16 && facade_comparisons == 20, "gate.nonvacuity");
  std::printf("{\"status\":\"passed\",\"authority\":\"independent_structural_prefix_expectations\","
      "\"encodings\":4,\"prefixes\":20,\"checks\":%zu,\"cut_root_and_read_queries\":%zu,"
      "\"invariant_cut_pairs\":%zu,\"successor_writes\":%zu,\"closed_changes\":%zu,"
      "\"rejects\":%zu,\"same_kernel_facade_comparisons\":%zu}\n",
      checks, cut_queries, invariant_queries, successor_writes, closed_changes,
      rejected_suffixes, facade_comparisons);
  return 0;
}
}  // namespace

int main(int argc, char**) {
  if (argc != 1) { std::fputs("FAIL invalid_arguments\n", stderr); return 2; }
  try { return run(); }
  catch (const Failure& failure) { std::fprintf(stderr, "FAIL %s\n", failure.reason); return 1; }
  catch (...) { std::fputs("FAIL unexpected_exception\n", stderr); return 1; }
}
