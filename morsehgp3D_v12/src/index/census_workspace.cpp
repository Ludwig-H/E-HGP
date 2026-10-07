// Un seul parcours : I croissant a gauche, U empilee a droite puis inversee. Aucune allocation par requete.
#include "index/access.hpp"
#include <algorithm>
#include <new>

namespace mhgp12 {
namespace {
class ActiveQuery {
 public:
  explicit ActiveQuery(std::atomic_flag& flag) noexcept : flag_(flag) {}
  ~ActiveQuery() { flag_.clear(std::memory_order_release); }
  ActiveQuery(const ActiveQuery&) = delete;
  ActiveQuery& operator=(const ActiveQuery&) = delete;
 private:
  std::atomic_flag& flag_;
};

struct BorrowedPass {
  std::span<SiteIdx> storage;
  u32 threshold;
  CensusLedger ledger;
  u64 p = 0, m = 0;

  Outcome inside(u32 begin, u32 end) noexcept {
    const u64 count = std::min<u64>(end - begin, threshold - p);
    if (p > storage.size() || m > storage.size() - p || count > storage.size() - p - m)
      return fail(Reason::arithmetic_invariant);
    for (u64 i = 0; i < count; ++i) storage[p + i] = SiteIdx{static_cast<u32>(begin + i)};
    p += count;
    return {};
  }
  Outcome points(const Cloud& cloud, u32 begin, u32 end, const num::LatticeSphere& lattice) noexcept {
    for (u32 i = begin; i < end && p < threshold; ++i) {
      ++ledger.point_tests;
      auto point = num::Point::make(cloud.x()[i], cloud.y()[i], cloud.z()[i]);
      if (!point.ok()) return point.outcome();
      auto side = lattice.side(point.value());
      if (!side.ok()) return side.outcome();
      if (side.value() < 0) { MHGP12_TRY(inside(i, i + 1)); }
      else if (side.value() == 0) {
        if (p > storage.size() || m >= storage.size() - p) return fail(Reason::arithmetic_invariant);
        storage[storage.size() - 1 - m] = SiteIdx{i}; ++m;
      }
    }
    return {};
  }
  Outcome walk(const GlobalIndex& index, const num::Sphere& sphere) noexcept {
    ++ledger.passes;
    const num::LatticeSphere lattice(sphere);  // une preparation par parcours ; sites de l'index entiers
    const auto nodes = index_detail::Access::nodes(index);
    for (u64 cursor = 0; cursor < nodes.size() && p < threshold;) {
      const auto& node = nodes[cursor];
      ++ledger.nodes; ++ledger.bounds;
      auto signs = lattice.bound_signs(node.box);  // minorant sur sites entiers, majorant continu
      if (!signs.ok()) return signs.outcome();
      if (signs.value().lower > 0) {
        ++ledger.outside_blocks; cursor = node.escape;
      } else if (signs.value().upper < 0) {
        ++ledger.inside_blocks;
        MHGP12_TRY(inside(node.begin, node.end)); cursor = node.escape;
      } else if (node.end - node.begin <= index.leaf_size()) {
        MHGP12_TRY(points(index.cloud(), node.begin, node.end, lattice)); cursor = node.escape;
      } else { ++cursor; }
    }
    return {};
  }
};
}  // namespace

Result<std::unique_ptr<CensusWorkspace>> CensusWorkspace::make(const GlobalIndex& index, MemoryBudget& budget) noexcept {
  const u32 n = index.cloud().sites();
  if (n == 0) return fail(Reason::empty_input);
  MHGP12_TRY(budget.admit(u64{n} * sizeof(SiteIdx)));
  std::unique_ptr<CensusWorkspace> made(new (std::nothrow) CensusWorkspace(index));
  if (!made) return fail(Reason::memory_budget);
  MHGP12_TRY(made->storage_.allocate(n, budget));
  return made;
}

Outcome CensusWorkspace::query(const GlobalIndex& index, const num::Sphere& sphere, u32 threshold,
                               void* context, Callback callback) noexcept {
  if (active_.test_and_set(std::memory_order_acquire)) return fail(Reason::parameter_out_of_range);
  const ActiveQuery active(active_);
  if (&index != index_ || index.cloud().sites() != storage_.size() || threshold == 0 || callback == nullptr)
    return fail(Reason::parameter_out_of_range);
  BorrowedPass pass{storage_.span(), threshold, {}};
  MHGP12_TRY(pass.walk(index, sphere));
  const bool saturated = pass.p == threshold;
  auto shell = storage_.span().last(saturated ? 0 : pass.m);
  std::reverse(shell.begin(), shell.end());
  const BorrowedCensus result(saturated ? CensusKind::saturated : CensusKind::complete,
                              storage_.span().first(pass.p), shell, pass.ledger);
  try { return callback(context, result); }
  catch (const std::bad_alloc&) { return fail(Reason::memory_budget); }
  catch (...) { return fail(Reason::task_exception); }
}
}  // namespace mhgp12
