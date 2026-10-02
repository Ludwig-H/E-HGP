// Inventaire et ordre canonique des naissances ; pas de reutilisation des SiteIdx Morton comme ordre des centres.
#include "tower/forest_internal.hpp"

namespace mhgp11::tower_detail {

Result<std::array<u64, 2>> forest_capacities(u64 births) noexcept {
  if (births == 0) return fail(Reason::tower_invariant);
  if (births >= (u64{kNone} + 1) / 2) return fail(Reason::tower_capacity);
  return std::array<u64, 2>{2 * births - 1, 2 * births - 2};
}

Outcome add_cell_work(CellLedger& sum, const CellLedger& one) noexcept {
  MHGP11_TRY(cell_add(sum.combinations, one.combinations));
  MHGP11_TRY(cell_add(sum.passes, one.passes));
  MHGP11_TRY(cell_add(sum.trace_tests, one.trace_tests));
  MHGP11_TRY(cell_add(sum.meb_calls, one.meb_calls));
  DescentLedger a, b; a.part_meb = sum.meb; b.part_meb = one.meb;
  MHGP11_TRY(add_descent(a, b)); sum.meb = a.part_meb;
  return {};
}

namespace {
Outcome add_classification(ClassificationLedger& sum, const ClassificationLedger& one) noexcept {
  MHGP11_TRY(cell_add(sum.combinations, one.combinations));
  MHGP11_TRY(cell_add(sum.examined, one.examined));
  MHGP11_TRY(cell_add(sum.meb_calls, one.meb_calls));
  MHGP11_TRY(cell_add(sum.meb.presentations, one.meb.presentations));
  MHGP11_TRY(cell_add(sum.meb.nondegenerate, one.meb.nondegenerate));
  MHGP11_TRY(cell_add(sum.meb.positive, one.meb.positive));
  MHGP11_TRY(cell_add(sum.meb.containing, one.meb.containing));
  MHGP11_TRY(cell_add(sum.meb.comparisons, one.meb.comparisons));
  MHGP11_TRY(cell_add(sum.meb.diameter_pairs, one.meb.diameter_pairs));
  return cell_add(sum.meb.point_tests, one.meb.point_tests);
}
struct BirthRecord { num::Sphere sphere; u32 key; LevelRank rank; };
static_assert(sizeof(BirthRecord) <= 1024 && sizeof(ForestNode) <= 64 && sizeof(ForestState) <= 64);
Result<num::Sphere> birth_sphere(const FullDomain& domain, BallIdx ball) noexcept {
  const auto& support = domain.catalogue().balls_data()[idx(ball)];
  const auto& cloud = domain.index().cloud();
  std::array<num::Point, 4> points{};
  for (u8 j = 0; j < support.qmin; ++j) {
    const u32 s = idx(support.support[j]);
    auto point = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
    if (!point.ok()) return point.outcome();
    points[j] = point.value();
  }
  auto sphere = support.qmin == 2 ? num::Sphere::through(points[0], points[1]) : support.qmin == 3 ?
    num::Sphere::through(points[0], points[1], points[2]) :
    num::Sphere::through(points[0], points[1], points[2], points[3]);
  if (!sphere.ok()) return sphere.outcome();
  if (!sphere.value()) return fail(Reason::tower_invariant);
  return *sphere.value();
}
}  // namespace

Outcome ForestBuilder::classify() noexcept {
  const auto& cat = domain.catalogue();
  MHGP11_TRY(kinds.allocate(cat.balls(), budget));
  u64 count = k == 1 ? domain.index().cloud().sites() : 0;
  for (u32 b = 0; b < cat.balls(); ++b) {
    kinds[b] = 0;
    const auto& data = cat.balls_data()[b];
    if (u64{data.p} + data.qmin - 1 > k || u64{data.p} + data.m < k) continue;
    auto made = classify_cell(domain, BallIdx{b}, static_cast<Order>(k));
    if (!made.ok()) return made.outcome();
    MHGP11_TRY(cell_add(result.ledger_.classified_cells, 1));
    MHGP11_TRY(add_classification(result.ledger_.classification, made.value().ledger()));
    kinds[b] = made.value().kind() == CellKind::birth ? 1 : 2;
    if (kinds[b] == 1) MHGP11_TRY(cell_add(count, 1));
  }
  // Des naissances aux parents : chaque fusion consomme au moins deux composantes distinctes.
  const auto capacities = forest_capacities(count);
  if (!capacities.ok()) return capacities.outcome();
  result.births_ = static_cast<u32>(count);
  result.order_ = static_cast<Order>(k);
  return {};
}

Outcome ForestBuilder::births() noexcept {
  const u64 b = result.births_, capacity = 2 * b - 1, edge_capacity = 2 * b - 2;
  // Tous facteurs sont <2^32 ; tailles des enregistrements compilees fixes, produits en u64.
  const u64 bytes = capacity * sizeof(ForestNode) + edge_capacity * sizeof(NodeIdx) +
                    b * (sizeof(BirthEntry) + sizeof(BirthRecord));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.nodes_.allocate(capacity, budget));
  MHGP11_TRY(result.children_.allocate(edge_capacity, budget));
  MHGP11_TRY(result.lookup_.allocate(b, budget));
  Buffer<BirthRecord> records;
  MHGP11_TRY(records.allocate(b, budget));
  u64 written = 0;
  if (k == 1) {
    const auto& cloud = domain.index().cloud();
    for (u32 s = 0; s < cloud.sites(); ++s) {
      auto point = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
      if (!point.ok()) return point.outcome();
      records[written++] = {num::Sphere::point(point.value()), s, LevelRank{0}};
    }
  }
  for (u32 id = 0; id < kinds.size(); ++id) if (kinds[id] == 1) {
    if (written >= b || k == 1) return fail(Reason::tower_invariant);
    auto sphere = birth_sphere(domain, BallIdx{id});
    if (!sphere.ok()) return sphere.outcome();
    records[written++] = {sphere.value(), id, domain.catalogue().balls_data()[id].rank};
  }
  if (written != b) return fail(Reason::tower_invariant);
  result.ledger_.birth_presentations = b;
  forest_sort(records.span(), [&](const BirthRecord& a, const BirthRecord& c) noexcept {
    if (a.rank != c.rank) return idx(a.rank) < idx(c.rank);
    // Heapsort <4b*ceil(log2 b) comparaisons, b<2^31 ; ce compteur reste <2^39.
    ++result.ledger_.center_comparisons;
    return num::compare_centers(a.sphere, c.sphere) < 0;
  });
  for (u32 i = 0; i < b; ++i) {
    result.nodes_[i] = {records[i].rank, NodeIdx{kNone}, 0, 0, records[i].key};
    result.lookup_[i] = {records[i].key, NodeIdx{i}};
  }
  result.count_ = result.births_;
  forest_sort(result.lookup_.span(), [](const BirthEntry& a, const BirthEntry& c) noexcept { return a.key < c.key; });
  return {};  // records rendu AVANT les DSU et les cellules rejouees.
}

Result<OrderForest> ForestBuilder::run() noexcept {
  OrderTimings draft;
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(classify());
  if (timings != nullptr) { draft.classify_ns = stage->nanoseconds(); stage.emplace(); }
  MHGP11_TRY(births());
  if (timings != nullptr) { draft.births_ns = stage->nanoseconds(); stage.emplace(); }
  const u64 b = result.births_;
  MHGP11_TRY(budget.admit(b * (sizeof(ForestState) + sizeof(u32))));
  MHGP11_TRY(states.allocate(b, budget)); MHGP11_TRY(touched.allocate(b, budget));
  for (u32 i = 0; i < b; ++i) states[i] = {i, i, kNone, kNone, kNone, false};
  MHGP11_TRY(plateaus());
  const u32 root = find(0);
  for (u32 i = 1; i < b; ++i) if (find(i) != root) return fail(Reason::tower_invariant);
  result.root_ = NodeIdx{states[root].top};
  if (result.edges_ + 1 != result.count_) return fail(Reason::tower_invariant);
  if (timings != nullptr) { draft.plateaus_ns = stage->nanoseconds(); *timings = draft; }
  return std::move(result);
}

Result<OrderForest> build_forest(const FullDomain& domain, u32 k, MemoryBudget& budget, OrderTimings* timings,
                                DescentMemo* memo) noexcept {
  if (k == 0 || k > domain.catalogue().kmax() || k > domain.index().cloud().sites())
    return fail(Reason::parameter_out_of_range);
  if (memo != nullptr && !memo->belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  return ForestBuilder(domain, k, budget, timings, memo).run();
}

}  // namespace mhgp11::tower_detail
