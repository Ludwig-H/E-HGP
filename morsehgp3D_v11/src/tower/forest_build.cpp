// Inventaire et ordre canonique des naissances ; pas de reutilisation des SiteIdx Morton comme ordre des centres.
#include <algorithm>
#include "tower/forest_internal.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/regular_vertical_seeds.hpp"

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

void point_births(const Cloud& cloud, std::span<ForestNode> nodes, std::span<BirthEntry> lookup) noexcept {
  for (u32 s = 0; s < cloud.sites(); ++s) lookup[s] = {s, NodeIdx{kNone}};
  forest_sort(lookup, [&](const BirthEntry& a, const BirthEntry& b) noexcept {
    if (cloud.x()[a.key] != cloud.x()[b.key]) return cloud.x()[a.key] < cloud.x()[b.key];
    if (cloud.y()[a.key] != cloud.y()[b.key]) return cloud.y()[a.key] < cloud.y()[b.key];
    return cloud.z()[a.key] < cloud.z()[b.key];
  });
  for (u32 i = 0; i < cloud.sites(); ++i)
    nodes[i] = {LevelRank{0}, NodeIdx{kNone}, 0, 0, lookup[i].key};
}

Outcome ranked_births(const FullDomain& domain, std::span<const u8> kinds, std::span<ForestNode> nodes,
                      std::span<BirthRecord> scratch, ForestLedger& work) noexcept {
  const auto balls = domain.catalogue().balls_data();
  u64 written = 0;
  for (u32 b = 0; b < kinds.size(); ++b) if (kinds[b] == 1) {
    if (written >= nodes.size()) return fail(Reason::tower_invariant);
    nodes[written++] = {balls[b].rank, NodeIdx{kNone}, 0, 0, b};
  }
  if (written != nodes.size()) return fail(Reason::tower_invariant);
  for (u64 begin = 0; begin < nodes.size();) {
    u64 end = begin + 1;
    while (end < nodes.size() && nodes[end].rank == nodes[begin].rank) ++end;
    if (end < nodes.size() && idx(nodes[end].rank) < idx(nodes[begin].rank))
      return fail(Reason::tower_invariant);
    const u64 count = end - begin;
    if (count > 1) {
      if (count > scratch.size()) return fail(Reason::tower_invariant);
      auto records = scratch.first(count);
      for (u64 i = 0; i < count; ++i) {
        const auto& node = nodes[begin + i];
        auto sphere = birth_sphere(domain, BallIdx{node.birth_key});
        if (!sphere.ok()) return sphere.outcome();
        records[i] = {sphere.value(), node.birth_key, node.rank};
      }
      MHGP11_TRY(cell_add(work.birth_presentations, count));
      forest_sort(records, [&](const BirthRecord& a, const BirthRecord& c) noexcept {
        ++work.center_comparisons;  // Somme <4B*ceil(log2 B), toujours <2^39.
        return num::compare_centers(a.sphere, c.sphere) < 0;
      });
      for (u64 i = 0; i < count; ++i)
        nodes[begin + i] = {records[i].rank, NodeIdx{kNone}, 0, 0, records[i].key};
    }
    begin = end;
  }
  return {};
}
}  // namespace

void BirthRuns::add(LevelRank rank) noexcept {
  if (!any) {
    any = uniform = true;
    head_rank = tail_rank = rank;
    head = tail = largest = 1;
    return;
  }
  if (rank == tail_rank) {
    ++tail;
    if (uniform) ++head;
  } else {
    uniform = false;
    tail_rank = rank;
    tail = 1;
  }
  largest = std::max(largest, tail);
}

void BirthRuns::append(const BirthRuns& next) noexcept {
  if (!next.any) return;
  if (!any) { *this = next; return; }
  const bool joined = tail_rank == next.head_rank;
  largest = std::max({largest, next.largest, joined ? tail + next.head : 0});
  if (uniform && joined) head += next.head;
  tail = next.uniform && joined ? tail + next.tail : next.tail;
  tail_rank = next.tail_rank;
  uniform = uniform && next.uniform && joined;
}

Outcome classify_range(const FullDomain& domain, u32 k, std::span<u8> kinds, u32 begin, u32 end,
                       ClassifyCounts& out) noexcept {
  const auto& cat = domain.catalogue();
  for (u32 b = begin; b < end; ++b) {
    kinds[b] = 0;
    const auto& data = cat.balls_data()[b];
    if (u64{data.p} + data.qmin - 1 > k || u64{data.p} + data.m < k) continue;
    if (data.m == data.qmin) {
      // Catalogue certifie : q=2..4, centre dans le simplexe strict. La fenetre ne contient
      // que h-1 (ses q faces strictes) et h=p+q (U entier, naissance), meme si p>0.
      // Les neuf autres compteurs de classification sont nuls : ne pas effacer le travail anterieur.
      const bool birth = k == u64{data.p} + data.qmin;
      MHGP11_TRY(cell_add(out.classified, 1));
      MHGP11_TRY(cell_add(out.classification.combinations, birth ? 1 : data.qmin));
      kinds[b] = birth ? 1 : 2;
      if (!birth) MHGP11_TRY(cell_add(out.regular_jobs, 1));
    } else {
      auto made = classify_cell(domain, BallIdx{b}, static_cast<Order>(k));
      if (!made.ok()) return made.outcome();
      MHGP11_TRY(cell_add(out.classified, 1));
      MHGP11_TRY(add_classification(out.classification, made.value().ledger()));
      kinds[b] = made.value().kind() == CellKind::birth ? 1 : 2;
    }
    if (kinds[b] == 1) {
      MHGP11_TRY(cell_add(out.births, 1));
      out.runs.add(data.rank);
    }
  }
  return {};
}

Outcome add_classify_counts(ClassifyCounts& sum, const ClassifyCounts& part) noexcept {
  MHGP11_TRY(cell_add(sum.classified, part.classified));
  MHGP11_TRY(cell_add(sum.births, part.births));
  MHGP11_TRY(cell_add(sum.regular_jobs, part.regular_jobs));
  sum.runs.append(part.runs);  // plages consecutives, dans l'ordre des boules
  return add_classification(sum.classification, part.classification);
}

Outcome ForestBuilder::adopt(const ClassifyCounts& counts) noexcept {
  // Sommes exactes : un decoupage en blocs donne les memes compteurs que le parcours unique.
  MHGP11_TRY(cell_add(result.ledger_.classified_cells, counts.classified));
  MHGP11_TRY(add_classification(result.ledger_.classification, counts.classification));
  u64 count = k == 1 ? domain.index().cloud().sites() : 0;
  MHGP11_TRY(cell_add(count, counts.births));
  // Des naissances aux parents : chaque fusion consomme au moins deux composantes distinctes.
  const auto capacities = forest_capacities(count);
  if (!capacities.ok()) return capacities.outcome();
  result.births_ = static_cast<u32>(count);
  result.order_ = static_cast<Order>(k);
  regular_jobs = counts.regular_jobs;
  // Une naissance seule n'exige aucune Sphere : son rang exact suffit a la placer.
  birth_runs = k == 1 || counts.runs.largest < 2 ? 0 : counts.runs.largest;
  return {};
}

Outcome ForestBuilder::classify() noexcept {
  const auto& cat = domain.catalogue();
  MHGP11_TRY(kinds.allocate(cat.balls(), budget));
  ClassifyCounts counts;
  MHGP11_TRY(classify_range(domain, k, kinds.span(), 0, cat.balls(), counts));
  return adopt(counts);
}

u64 ForestBuilder::birth_bytes() const noexcept {
  const u64 b = result.births_, capacity = 2 * b - 1, edge_capacity = 2 * b - 2;
  const u64 run_capacity = birth_runs;  // calcule une fois a la classification
  const u64 dense_capacity = !dense_birth_lookup ? 0 : k == 1 ? domain.index().cloud().sites() : domain.catalogue().balls();
  const u64 sparse_capacity = !dense_birth_lookup || k == 1 ? b : 0;
  // Tous facteurs sont <2^32 ; tailles des enregistrements compilees fixes, produits en u64.
  return capacity * sizeof(ForestNode) + edge_capacity * sizeof(NodeIdx) +
         sparse_capacity * sizeof(BirthEntry) + dense_capacity * sizeof(NodeIdx) +
         run_capacity * sizeof(BirthRecord);
}

Outcome ForestBuilder::births() noexcept {
  const u64 b = result.births_, capacity = 2 * b - 1, edge_capacity = 2 * b - 2;
  const u64 run_capacity = birth_runs;
  const u64 dense_capacity = !dense_birth_lookup ? 0 : k == 1 ? domain.index().cloud().sites() : domain.catalogue().balls();
  const u64 sparse_capacity = !dense_birth_lookup || k == 1 ? b : 0;
  MHGP11_TRY(budget.admit(birth_bytes()));
  MHGP11_TRY(result.nodes_.allocate(capacity, budget));
  MHGP11_TRY(result.children_.allocate(edge_capacity, budget));
  MHGP11_TRY(result.lookup_.allocate(sparse_capacity, budget));
  MHGP11_TRY(result.dense_.allocate(dense_capacity, budget));
  Buffer<BirthRecord> records;
  MHGP11_TRY(records.allocate(run_capacity, budget));
  if (k == 1) {
    if (b != domain.index().cloud().sites()) return fail(Reason::tower_invariant);
    point_births(domain.index().cloud(), result.nodes_.span().first(b), result.lookup_.span());
  } else MHGP11_TRY(ranked_births(domain, kinds.span(), result.nodes_.span().first(b), records.span(), result.ledger_));
  if (dense_birth_lookup) {
    for (auto& node : result.dense_.span()) node = NodeIdx{kNone};
    for (u32 i = 0; i < b; ++i) {
      const u32 key = result.nodes_[i].birth_key;
      if (key >= dense_capacity || result.dense_[key] != NodeIdx{kNone}) return fail(Reason::tower_invariant);
      result.dense_[key] = NodeIdx{i};
    }
    result.lookup_.reset();  // Scratch XYZ de K1 rendu ; jamais deux lookups retenus.
  } else {
    for (u32 i = 0; i < b; ++i) result.lookup_[i] = {result.nodes_[i].birth_key, NodeIdx{i}};
    forest_sort(result.lookup_.span(), [](const BirthEntry& a, const BirthEntry& c) noexcept { return a.key < c.key; });
  }
  result.count_ = result.births_;
  return {};  // records rendu AVANT les DSU et les cellules rejouees.
}

Result<OrderForest> ForestBuilder::run() noexcept {
  if (vertical_seeds != nullptr && !vertical_seeds->belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  OrderTimings draft;
  parallel_timings = &draft;
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(classify());
  if (timings != nullptr) { draft.classify_ns = stage->nanoseconds(); stage.emplace(); }
  MHGP11_TRY(births());
  if (timings != nullptr) { draft.births_ns = stage->nanoseconds(); stage.emplace(); }
  MHGP11_TRY(prepare_states());
  MHGP11_TRY(plateaus());
  MHGP11_TRY(finish());
  if (timings != nullptr) { draft.plateaus_ns = stage->nanoseconds(); *timings = draft; }
  return std::move(result);
}

Outcome ForestBuilder::prepare_states() noexcept {
  const u64 b = result.births_;
  MHGP11_TRY(budget.admit(b * (sizeof(ForestState) + sizeof(u32))));
  MHGP11_TRY(states.allocate(b, budget)); MHGP11_TRY(touched.allocate(b, budget));
  for (u32 i = 0; i < b; ++i) states[i] = {i, i, kNone, kNone, kNone, false};
  return {};
}

Outcome ForestBuilder::finish() noexcept {
  const u32 b = result.births_;
  const u32 root = find(0);
  for (u32 i = 1; i < b; ++i) if (find(i) != root) return fail(Reason::tower_invariant);
  result.root_ = NodeIdx{states[root].top};
  if (result.edges_ + 1 != result.count_) return fail(Reason::tower_invariant);
  return {};
}

Result<OrderForest> build_forest(const FullDomain& domain, u32 k, MemoryBudget& budget, OrderTimings* timings,
                                DescentMemo* memo, ForestParallel* parallel, bool dense_birth_lookup) noexcept {
  if (k == 0 || k > domain.catalogue().kmax() || k > domain.index().cloud().sites())
    return fail(Reason::parameter_out_of_range);
  if (memo != nullptr && !memo->belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  if (parallel != nullptr && !parallel->belongs_to(domain, budget)) return fail(Reason::parameter_out_of_range);
  return ForestBuilder(domain, k, budget, timings, memo, parallel, dense_birth_lookup).run();
}

}  // namespace mhgp11::tower_detail
