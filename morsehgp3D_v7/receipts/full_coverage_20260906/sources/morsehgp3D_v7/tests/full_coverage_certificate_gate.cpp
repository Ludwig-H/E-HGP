// Structural journal vs independent forward set replay and bounded Gram/Gamma.
// Geometry fixtures are test inputs, never a new producer authority.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string_view>
#include <type_traits>

#include "../src/forest/full_coverage_certificate.hpp"
#include "../oracle/local_plateau_oracle.hpp"

#ifdef MHGP7_TESTING
#error This gate must use the unmodified product path
#endif

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
}
void* operator new(size_t n) { return allocation_fault::allocate(n); }
void* operator new[](size_t n) { return allocation_fault::allocate(n); }
void operator delete(void* p) noexcept { allocation_fault::release(p); }
void operator delete[](void* p) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t) noexcept { allocation_fault::release(p); }

namespace {
using namespace mhgp7;
namespace oracle = local_plateau_oracle;
struct Failure { const char* why; };
size_t checks = 0, rejects = 0, cuts = 0, gamma_cuts = 0, allocation_rejects = 0;
void need(bool value, const char* why) { ++checks; if (!value) throw Failure{why}; }
ExactLevel level(u64 n, i128 d = 1) { return {{n, 0, 0}, d}; }
using Bank = std::shared_ptr<const FullCoveragePopulations>;
Bank bank(const std::vector<PointId>& domain, const std::vector<FullCoveragePopulation>& rows) {
  auto result = build_full_coverage_populations(domain, rows);
  need(result.status == FullCertificateStatus::kOk && result.value, "bank.valid");
  return result.value;
}
bool empty(const FullCoverageCertificate& value) {
  return value.order() == 0 && !value.populations() && value.nodes().empty() &&
      value.parents().empty() && value.successors().empty() && value.contributions().empty();
}
FullCoverageCertificate build(unsigned k, Bank store, const std::vector<FullCoverageBatch>& batches) {
  auto result = build_full_coverage_certificate(k, std::move(store), batches);
  need(result.status == FullCertificateStatus::kOk && result.value.order() == k &&
      std::string_view(result.reason) == "structural_only", "journal.valid");
  return std::move(result.value);
}
std::vector<PointId> read(const FullCoverageCertificate& forest, FullNodeId root,
    ExactLevel cut, bool closed) {
  auto result = full_coverage_at(forest, root, cut, closed);
  need(result.status == FullCertificateStatus::kOk, "reader.valid");
  return result.values;
}
void reject(unsigned k, Bank store, const std::vector<FullCoverageBatch>& batches, const char* reason) {
  auto result = build_full_coverage_certificate(k, std::move(store), batches);
  need(result.status == FullCertificateStatus::kInvalidInput && empty(result.value) &&
      std::string_view(result.reason) == reason, reason);
  ++rejects;
}

// Independent algorithm: forward point sets, no successor arena and no reader
// helper. It deliberately spends the point-set memory forbidden to the producer.
void replay(const FullCoverageCertificate& forest, const std::vector<FullCoverageBatch>& batches,
    u64 numerator, u64 denominator, bool closed) {
  std::map<FullNodeId, std::set<PointId>> roots;
  FullNodeId next = 0;
  for (const auto& batch : batches) {
    // These fixtures use small rational levels; this comparison is independent
    // of the product's U320 level comparison and admission helper.
    need(batch.level.num[1] == 0 && batch.level.num[2] == 0, "replay.small_fixture");
    const u64 lhs = batch.level.num[0] * denominator;
    const u64 rhs = numerator * static_cast<u64>(batch.level.den);
    if (lhs > rhs || (!closed && lhs == rhs)) break;
    for (const auto& action : batch.actions) {
      std::set<PointId> points;
      for (auto parent : action.parents) {
        need(roots.contains(parent), "replay.parent");
        points.insert(roots.at(parent).begin(), roots.at(parent).end());
        roots.erase(parent);
      }
      for (const auto& ref : action.contributions) {
        const auto& row = forest.populations()->rows().at(ref.population);
        if (ref.include_interior) points.insert(row.interior.begin(), row.interior.end());
        for (size_t bit = 0; bit < row.shell.size(); ++bit)
          if (ref.shell_mask & (u16{1} << bit)) points.insert(row.shell[bit]);
      }
      const auto id = action.parents.size() == 1 ? action.parents.front() : next++;
      roots.emplace(id, std::move(points));
    }
  }
  const auto cut = level(numerator, denominator);
  for (FullNodeId id = 0; id < forest.nodes().size(); ++id) {
    const bool live = full_coverage_root_at(forest, id, cut, closed) == id;
    need(live == roots.contains(id), "replay.live_identity");
    if (live) need(read(forest, id, cut, closed) ==
        std::vector<PointId>(roots.at(id).begin(), roots.at(id).end()), "replay.coverage");
    else need(full_coverage_at(forest, id, cut, closed).status == FullCertificateStatus::kInvalidInput,
        "replay.dead_root_rejected");
  }
  ++cuts;
}

void gamma(const FullCoverageCertificate& forest, const std::vector<P3>& points,
    std::initializer_list<u64> levels) {
  oracle::Model model(points);
  for (u64 t : levels) for (bool closed : {false, true}) {
    std::vector<std::vector<PointId>> expected, actual;
    for (const auto& component : model.components(forest.order(), oracle::Rat(t), closed,
        (u32{1} << points.size()) - 1)) {
      u32 mask = 0; for (auto facet : component) mask |= facet;
      std::vector<PointId> cover;
      for (size_t bit = 0; bit < points.size(); ++bit) if (mask & (u32{1} << bit)) cover.push_back(bit);
      expected.push_back(std::move(cover));
    }
    for (size_t id = 0; id < forest.nodes().size(); ++id)
      if (full_coverage_root_at(forest, id, level(t), closed) == id)
        actual.push_back(read(forest, id, level(t), closed));
    std::sort(expected.begin(), expected.end()); std::sort(actual.begin(), actual.end());
    need(actual == expected, "gram_gamma.coverage_multiset");
    ++gamma_cuts;
  }
}

void geometry_fixtures() {
  const auto square_bank = bank({0,1,2,3}, {{{}, {0,1,2,3}}});
  const std::vector<FullCoverageBatch> square{{level(2), {{{}, {{0,15,false}}}}}};
  auto k3 = build(3, square_bank, square), terminal = build(4, square_bank, square);
  need(k3.populations().get() == terminal.populations().get(), "bank.shared_across_orders");
  need(k3.nodes().size() == 1 && read(k3, 0, level(2), true).size() == 4,
       "birth.coverage_exceeds_K");
  const std::vector<P3> square_xyz{{0,0,0},{2,0,0},{2,2,0},{0,2,0}};
  gamma(k3, square_xyz, {0,1,2,3}); gamma(terminal, square_xyz, {0,1,2,3});
  const auto growth_bank = bank({0,1,2,3}, {{{1},{0,2}}, {{},{0,1,2,3}}});
  std::vector<FullCoverageBatch> growth{{level(16), {{{}, {{0,3,true}}}}},
      {level(25), {{{0}, {{1,8,false}}}}}};
  auto forest = build(3, growth_bank, growth);
  need(forest.nodes().size() == 1 && forest.contributions().size() == 2, "growth.no_fake_node");
  need(read(forest, 0, level(25), false) == std::vector<PointId>({0,1,2}), "growth.no_future_leak");
  need(read(forest, 0, level(25), true) == std::vector<PointId>({0,1,2,3}), "growth.dated_contribution");
  gamma(forest, {{1,8,0},{5,10,0},{9,8,0},{5,0,0}}, {0,15,16,24,25,26});
  growth[1].actions[0].contributions[0].shell_mask = 15;  // redundant full S, same union
  auto redundant = build(3, growth_bank, growth);
  for (u64 t : {16,24,25,26}) for (bool closed : {false,true})
    if (full_coverage_root_at(forest, 0, level(t), closed) == 0)
      need(read(forest, 0, level(t), closed) == read(redundant, 0, level(t), closed), "growth.not_disjoint_delta");
}

void structural_fixtures() {
  const auto store = bank({0,1,2,3,4,5}, {{{0},{1,2}}, {{2},{3,4}},
      {{},{0,1,2,3,4,5}}, {{},{3,4,5}}});
  const std::vector<FullCoverageBatch> batches{
      {level(1), {{{}, {{0,3,true}}}, {{}, {{1,3,true}}}}},
      {level(2), {{{0}, {{2,8,false}}}}},
      {level(3), {{{}, {{3,7,false}}}}},
      {level(4), {{{0,1}, {{2,32,false}}}}},
      {level(5), {{{3}, {{2,32,false}, {2,32,false}}}}},
      {level(6), {{{2,3}, {}}}}};
  auto forest = build(3, store, batches);
  need(forest.nodes().size() == 5 && forest.parents().size() == 4 &&
      forest.contributions().size() == 7, "storage.factorized_shape");
  for (u64 t = 0; t <= 14; ++t) for (bool closed : {false,true}) replay(forest, batches, t, 2, closed);
  need(full_coverage_root_at(forest, 0, level(4), false) == 0 &&
      full_coverage_root_at(forest, 0, level(4), true) == 3 &&
      full_coverage_root_at(forest, 0, level(6), true) == 4, "history.not_final_root");
  need(read(forest, 0, level(3), true) == std::vector<PointId>({0,1,2,3}) &&
      read(forest, 1, level(3), true) == std::vector<PointId>({2,3,4}), "overlap.distinct_components");
  need(full_coverage_root_at(forest, 5, level(10), true) == kFullCoverageAbsent &&
      full_coverage_at(forest, kFullCoverageAbsent, level(10), true).values.empty(), "reader.bad_id");
  need(full_coverage_root_at(forest, 0, level(1,0), true) == kFullCoverageAbsent, "reader.bad_cut");
  FullCoverageCertificate moved(std::move(forest));
  need(empty(forest) && moved.order() == 3, "move.invalidates_source");
  forest = std::move(moved); need(empty(moved) && forest.order() == 3, "move_assignment.invalidates_source");

  reject(0, store, batches, "coverage_invalid_domain");
  reject(3, {}, batches, "coverage_invalid_domain");
  reject(3, std::make_shared<FullCoveragePopulations>(), batches, "coverage_invalid_domain");
  reject(3, store, {}, "coverage_invalid_domain");
  auto bad = batches; bad[1].level.den = 0; reject(3, store, bad, "coverage_invalid_level");
  bad = batches; bad[1].level = level(2,2); reject(3, store, bad, "coverage_nonincreasing_batch");
  bad = batches; bad[0].level = level(0); reject(3, store, bad, "coverage_positive_level_required");
  bad = batches; bad[1].actions.clear(); reject(3, store, bad, "coverage_empty_batch");
  bad = batches; bad[1].actions[0].contributions.clear(); reject(3, store, bad, "coverage_empty_continuation");
  bad = batches; bad[1].actions.push_back(bad[1].actions.front());
  reject(3, store, bad, "coverage_parent_not_unique_prebatch_root");
  for (const std::vector<FullNodeId>& parents : {std::vector<FullNodeId>{0,0}, {1,0}, {0,2}}) {
    bad = batches; bad[1].actions[0].parents = parents;
    reject(3, store, bad, "coverage_parent_not_unique_prebatch_root");
  }
  bad = batches; bad[4].actions[0].parents = {0}; reject(3, store, bad, "coverage_parent_not_unique_prebatch_root");
  bad = batches; bad[0].actions[1].parents = {0}; reject(3, store, bad, "coverage_parent_not_unique_prebatch_root");
  bad = batches; bad[1].actions[0].contributions[0].population = 4;
  reject(3, store, bad, "coverage_population_reference");
  for (FullCoverageRef ref : {FullCoverageRef{2,64,false}, {2,0,false}, {2,1,true}}) {
    bad = batches; bad[1].actions[0].contributions[0] = ref;
    reject(3, store, bad, "coverage_empty_or_invalid_mask");
  }
  bad = batches; bad[0].actions[0].contributions[0].shell_mask = 1;
  reject(3, store, bad, "coverage_birth_population");
  bad = batches; bad[0].actions[0].contributions.clear(); reject(3, store, bad, "coverage_birth_population");

  // Every observed allocation in bank/build/read is denied persistently once.
  const auto fault_loop = [](auto operation, auto refused) {
    allocation_fault::calls = 0; allocation_fault::count = true;
    { auto result = operation(); (void)result; }
    allocation_fault::count = false;
    const auto calls = allocation_fault::calls; need(calls > 0, "allocation.nonvacuous");
    for (size_t i = 0; i < calls; ++i) {
      allocation_fault::remaining = static_cast<long>(i);
      auto result = operation(); allocation_fault::remaining = -1;
      need(refused(result), "allocation.fail_closed"); ++allocation_rejects;
    }
  };
  fault_loop([&] { return build_full_coverage_populations(store->domain(), store->rows()); },
      [](const auto& r) { return r.status == FullCertificateStatus::kResourceExhausted && !r.value; });
  fault_loop([&] { return build_full_coverage_certificate(3, store, batches); },
      [](const auto& r) { return r.status == FullCertificateStatus::kResourceExhausted && empty(r.value); });
  fault_loop([&] { return full_coverage_at(forest, 4, level(7), true); },
      [](const auto& r) { return r.status == FullCertificateStatus::kResourceExhausted && r.values.empty(); });
}

void domain_and_k1() {
  const auto store = bank({0,1}, {{{},{0}}, {{},{1}}});
  const std::vector<FullCoverageBatch> batches{{level(0), {{{},{{0,1,false}}}, {{},{{1,1,false}}}}},
      {level(1), {{{0,1},{}}}}};
  auto forest = build(1, store, batches);
  gamma(forest, {{0,0,0},{2,0,0}}, {0,1,2});
  auto bad = batches; bad[0].actions[1].contributions = {{0,1,false}};
  reject(1, store, bad, "coverage_k1_roots");
  bad = batches; bad[0].level = level(1); reject(1, store, bad, "coverage_k1_roots");
  bad = batches; bad.push_back({level(2), {{{},{{0,1,false}}}}}); reject(1, store, bad, "coverage_k1_roots");
  for (const auto& rows : std::vector<std::vector<FullCoveragePopulation>>{
      {}, {{{},{}}}, {{{},{0,0}}}, {{{},{1,0}}}, {{{0},{0}}}, {{{},{2}}}}) {
    const auto result = build_full_coverage_populations(store->domain(), rows);
    need(result.status == FullCertificateStatus::kInvalidInput && !result.value, "bank.reject"); ++rejects;
  }
  const std::vector<PointId> reversed{1,0};
  need(!build_full_coverage_populations(reversed, store->rows()).value, "bank.domain_order");
  std::vector<PointId> large(5000); for (size_t i = 0; i < large.size(); ++i) large[i] = i;
  auto large_bank = bank(large, {{large,{}}});
  auto large_forest = build(10, large_bank, {{level(1), {{{},{{0,0,true}}}}}});
  need(read(large_forest, 0, level(1), true) == large, "bank.no_interior_work_ceiling");
  std::vector<PointId> shell(16); for (size_t i = 0; i < shell.size(); ++i) shell[i] = i;
  auto mask_bank = bank(large, {{{},shell}});
  auto mask_forest = build(10, mask_bank, {{level(1), {{{},{{0,65535,false}}}}}});
  need(read(mask_forest, 0, level(1), true) == shell, "bank.full_mask_width");
  shell.push_back(16);
  need(!build_full_coverage_populations(large, std::vector<FullCoveragePopulation>{{{},shell}}).value,
      "bank.representation_width_reject");
}

static_assert(!std::is_copy_constructible_v<FullCoveragePopulations>);
static_assert(!std::is_copy_assignable_v<FullCoveragePopulations>);
static_assert(!std::is_move_assignable_v<FullCoveragePopulations>);
static_assert(!std::is_copy_constructible_v<FullCoverageCertificate>);
static_assert(std::is_nothrow_move_constructible_v<FullCoverageCertificate>);
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    geometry_fixtures(); structural_fixtures(); domain_and_k1();
    need(cuts == 30 && gamma_cuts == 34 && rejects >= 30 && allocation_rejects >= 20,
         "nonvacuity.floor");
    std::printf("full_coverage_certificate checks=%zu rejects=%zu replay_cuts=%zu gamma_cuts=%zu allocation_rejects=%zu authority=structural_only\n",
        checks, rejects, cuts, gamma_cuts, allocation_rejects);
    return 0;
  } catch (const Failure& error) {
    allocation_fault::remaining = -1;
    std::fprintf(stderr, "FAIL %s\n", error.why); return 1;
  } catch (const std::exception& error) {
    allocation_fault::remaining = -1;
    std::fprintf(stderr, "EXCEPTION %s\n", error.what()); return 1;
  }
}
