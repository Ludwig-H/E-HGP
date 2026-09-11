#include <cstdio>
#include <string_view>

#include "variant/morsehgp3D_v7/src/forest/full_coverage_incremental.hpp"
// The predecessor headers are byte-identical in the two snapshots. GCC may
// honor pragma-once by deduplicating them even across the copied paths; import
// their unchanged types explicitly. The baseline JOURNAL below is not changed.
namespace mhgp7_baseline { using namespace mhgp7; }
#define mhgp7 mhgp7_baseline
#include "baseline/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"
#undef mhgp7

namespace {
using namespace mhgp7;
using namespace mhgp7::full_coverage_detail;
namespace baseline = mhgp7_baseline;
struct Failure { const char* reason; };
size_t checks = 0, compared_nodes = 0, compared_parents = 0, compared_contributions = 0;
size_t stability_cuts = 0, successor_transitions = 0;
void need(bool condition, const char* reason) { ++checks; if (!condition) throw Failure{reason}; }
ExactLevel level(u64 value) { return {{value, 0, 0}, 1}; }

void paired(unsigned order, const std::vector<PointId>& domain,
    const std::vector<FullCoveragePopulation>& rows, const std::vector<FullCoverageBatch>& batches) {
  std::vector<baseline::FullCoveragePopulation> base_rows;
  for (const auto& row : rows) base_rows.push_back({row.interior, row.shell});
  std::vector<baseline::FullCoverageBatch> base_batches;
  for (const auto& batch : batches) {
    baseline::FullCoverageBatch copied;
    std::copy(std::begin(batch.level.num), std::end(batch.level.num), std::begin(copied.level.num));
    copied.level.den = batch.level.den;
    for (const auto& action : batch.actions) {
      baseline::FullCoverageAction copied_action; copied_action.parents = action.parents;
      for (const auto& ref : action.contributions)
        copied_action.contributions.push_back({ref.population, ref.shell_mask, ref.include_interior});
      copied.actions.push_back(std::move(copied_action));
    }
    base_batches.push_back(std::move(copied));
  }
  auto bank = baseline::build_full_coverage_populations(domain, base_rows);
  auto expected = baseline::build_full_coverage_certificate(order, bank.value, base_batches);
  need(expected.status == baseline::FullCertificateStatus::kOk, "original.baseline.success");
  CoverageTowerAssembler owner(domain);
  for (const auto& row : rows)
    need(owner.append_population(row).status == FullCertificateStatus::kOk, "incremental.row");
  const auto started = owner.begin_order(order);
  need(started.status == FullCertificateStatus::kOk, "incremental.order");
  FullNodeId next = 0;
  for (const auto& batch : batches) {
    const auto appended = owner.append_batch(started.token, batch);
    size_t new_count = 0;
    for (const auto& action : batch.actions) if (action.parents.size() != 1) ++new_count;
    need(appended.status == FullCertificateStatus::kOk && appended.first_new_node == next &&
        appended.new_node_count == new_count, "incremental.ids.match.actions");
    next += new_count;
  }
  auto actual = owner.seal();
  need(actual.status == FullCertificateStatus::kOk && actual.orders.size() == 1, "incremental.seal");
  const auto& a = expected.value; const auto& b = actual.orders[0];
  need(a.order() == b.order() && a.nodes().size() == b.nodes().size() &&
      a.parents() == b.parents() && a.successors() == b.successors() &&
      a.contributions().size() == b.contributions().size(), "physical.arena.shapes_and_arcs");
  need(a.populations()->domain() == b.populations()->domain() &&
      a.populations()->rows().size() == b.populations()->rows().size(), "physical.bank.shapes");
  for (size_t i = 0; i < rows.size(); ++i)
    need(a.populations()->rows()[i].interior == b.populations()->rows()[i].interior &&
        a.populations()->rows()[i].shell == b.populations()->rows()[i].shell, "physical.bank.rows");
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    need(std::equal(std::begin(x.level.num), std::end(x.level.num), std::begin(y.level.num)) &&
        x.level.den == y.level.den &&
        x.first == y.first && x.parent_count == y.parent_count, "physical.node");
    ++compared_nodes;
  }
  compared_parents += a.parents().size();
  for (size_t i = 0; i < a.contributions().size(); ++i) {
    const auto& x = a.contributions()[i]; const auto& y = b.contributions()[i];
    need(std::equal(std::begin(x.level.num), std::end(x.level.num), std::begin(y.level.num)) &&
        x.level.den == y.level.den && x.segment == y.segment &&
        x.ref.population == y.ref.population && x.ref.shell_mask == y.ref.shell_mask &&
        x.ref.include_interior == y.ref.include_interior, "physical.contribution");
    ++compared_contributions;
  }
}

u64 draw(u64& seed) {
  seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return seed;
}

// Fixture transcribed from the independent auditor's structural_case(false, 1)
// in audits/receipts_coverage_cpp_20260910/corpus.py. These are separate complete
// constructions of each valid prefix, NOT publication of a pending prefix.
void prefix_stability() {
  const std::vector<PointId> domain{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const std::vector<FullCoveragePopulation> rows{
      {{0}, {1}}, {{}, {1, 2}}, {{}, {3, 4}}, {{}, {4, 5}}, {{6}, {7}}, {{}, domain}};
  const std::vector<FullCoverageBatch> batches{
      {level(1), {{{}, {{0, 1, true}}}, {{}, {{1, 3, false}}},
                   {{}, {{2, 3, false}}}, {{}, {{3, 3, false}}}}},
      {level(2), {{{}, {{4, 1, true}}}, {{3}, {{5, 256, false}, {5, 256, false}}},
                   {{0, 1, 2}, {{5, 512, false}}}}},
      {level(3), {{{4}, {{5, 1, false}}}}},
      {level(4), {{{3, 4, 5}, {}}}},
      {level(5), {{{6}, {{5, 1023, false}, {5, 256, false}}}}}};
  const auto build_prefix = [&](size_t count) {
    CoverageTowerAssembler owner(domain);
    for (const auto& row : rows)
      need(owner.append_population(row).status == FullCertificateStatus::kOk, "prefix.row");
    const auto order = owner.begin_order(2);
    for (size_t i = 0; i < count; ++i)
      need(owner.append_batch(order.token, batches[i]).status == FullCertificateStatus::kOk,
          "prefix.append");
    auto sealed = owner.seal();
    need(sealed.status == FullCertificateStatus::kOk, "prefix.seal");
    return std::move(sealed.orders[0]);
  };
  auto previous = build_prefix(1);
  for (size_t count = 2; count <= batches.size(); ++count) {
    auto current = build_prefix(count);
    paired(2, domain, rows,
        std::vector<FullCoverageBatch>(batches.begin(), batches.begin() + static_cast<ptrdiff_t>(count)));
    // Append-only logical arenas; successor slots have a distinct one-write rule.
    need(std::equal(previous.parents().begin(), previous.parents().end(), current.parents().begin()),
        "prefix.parents.preserved");
    for (size_t i = 0; i < previous.nodes().size(); ++i) {
      const auto& a = previous.nodes()[i]; const auto& b = current.nodes()[i];
      need(a.level == b.level && a.first == b.first && a.parent_count == b.parent_count,
          "prefix.nodes.preserved");
      const auto before = previous.successors()[i], after = current.successors()[i];
      bool newly_consumed = false;
      for (const auto& action : batches[count - 1].actions)
        if (action.parents.size() > 1 &&
            std::find(action.parents.begin(), action.parents.end(), i) != action.parents.end())
          newly_consumed = true;
      if (newly_consumed) {
        need(before == kFullCoverageAbsent && after >= previous.nodes().size() &&
            after < current.nodes().size() && current.nodes()[after].level == batches[count - 1].level,
            "prefix.successor.exactly.once.to.new.merge");
        ++successor_transitions;
      } else need(before == after, "prefix.successor.not.rewritten");
    }
    for (size_t i = 0; i < previous.contributions().size(); ++i) {
      const auto& a = previous.contributions()[i]; const auto& b = current.contributions()[i];
      need(a.level == b.level && a.segment == b.segment && a.ref.population == b.ref.population &&
          a.ref.shell_mask == b.ref.shell_mask && a.ref.include_interior == b.ref.include_interior,
          "prefix.contributions.preserved");
    }
    for (u64 twice = 0; twice <= 2 * count; ++twice) for (bool closed : {false, true}) {
      if (twice == 2 * count && closed) continue;
      auto cut = level(twice); cut.den = 2;
      for (FullNodeId id = 0; id < current.nodes().size(); ++id) {
        const auto before = full_coverage_root_at(previous, id, cut, closed);
        const auto after = full_coverage_root_at(current, id, cut, closed);
        need(before == after, "prefix.old.cut.root.stable");
        if (after == id) {
          const auto old_cover = full_coverage_at(previous, id, cut, closed);
          const auto new_cover = full_coverage_at(current, id, cut, closed);
          need(old_cover.status == FullCertificateStatus::kOk &&
              new_cover.status == FullCertificateStatus::kOk && old_cover.values == new_cover.values,
              "prefix.old.cut.coverage.stable");
        }
      }
      ++stability_cuts;
    }
    previous = std::move(current);
  }
  need(successor_transitions == 6 && stability_cuts == 60, "prefix.nonvacuity");
}
int run() {
  std::vector<PointId> domain;
  std::vector<FullCoveragePopulation> rows;
  for (PointId p = 0; p < 16; ++p) { domain.push_back(p); rows.push_back({{}, {p}}); }
  rows.push_back({{}, domain});
  rows.push_back({std::vector<PointId>(domain.begin(), domain.begin() + 8),
                  std::vector<PointId>(domain.begin() + 8, domain.end())});
  for (unsigned order = 1; order <= 10; ++order) for (u64 trial = 1; trial <= 10; ++trial) {
    u64 seed = 0x24b17aU + order * 317 + trial * 3719;
    FullNodeId next = 0;
    std::vector<FullNodeId> roots;
    std::vector<FullCoverageBatch> batches;
    if (order == 1) {
      FullCoverageBatch zero;
      for (u64 p = 0; p < 16; ++p) {
        zero.actions.push_back({{}, {{p, 1, false}}}); roots.push_back(next++);
      }
      batches.push_back(std::move(zero));
    }
    for (u64 step = 1; step <= 50; ++step) {
      FullCoverageBatch batch; batch.level = level(step);
      unsigned mode = static_cast<unsigned>(draw(seed) % 3);
      if (roots.empty()) mode = 0;
      else if (order == 1 && mode == 0) mode = roots.size() >= 2 ? 2 : 1;
      else if (roots.size() == 1 && mode == 2) mode = 1;
      FullCoverageAction action;
      if (mode == 0) {
        if (draw(seed) & 1) action.contributions.push_back({16, 65535, false});
        else action.contributions.push_back({17, 255, true});
        roots.push_back(next++);
      } else if (mode == 1) {
        action.parents.push_back(roots[draw(seed) % roots.size()]);
        action.contributions.push_back({17, static_cast<u16>(1U << (draw(seed) % 8)),
                                        (draw(seed) & 1) != 0});
      } else {
        size_t first = draw(seed) % roots.size();
        size_t second = draw(seed) % (roots.size() - 1);
        if (second >= first) ++second;
        if (first > second) std::swap(first, second);
        action.parents = {roots[first], roots[second]};
        roots.erase(roots.begin() + static_cast<ptrdiff_t>(second));
        roots.erase(roots.begin() + static_cast<ptrdiff_t>(first));
        roots.push_back(next++);
        if (draw(seed) & 1) action.contributions.push_back({16, 1, false});
      }
      batch.actions.push_back(std::move(action)); batches.push_back(std::move(batch));
    }
    paired(order, domain, rows, batches);
  }
  prefix_stability();
  need(compared_nodes > 2000 && compared_parents > 1500 && compared_contributions > 4000,
      "paired.nonvacuity");
  std::printf("{\"status\":\"passed\",\"cases\":104,\"checks\":%zu,"
      "\"nodes\":%zu,\"parents\":%zu,\"contributions\":%zu,"
      "\"stability_cuts\":%zu,\"successor_transitions\":%zu,"
      "\"authority\":\"structural_physical_equality_only\"}\n", checks,
      compared_nodes, compared_parents, compared_contributions, stability_cuts, successor_transitions);
  return 0;
}
}
int main(int argc, char**) {
  if (argc != 1) return 2;
  try { return run(); }
  catch (const Failure& error) { std::fprintf(stderr, "FAIL %s\n", error.reason); return 1; }
  catch (...) { std::fputs("FAIL unexpected_exception\n", stderr); return 1; }
}
