// Un seul parcours : I croissant a gauche, U empilee a droite puis inversee. Aucune allocation par requete. Parcours a
// plat (T2-d-B2) : aucune fonction appelee ni Result construit par noeud ou par site dans la voie gardee.
// Le meme parcours sert le census generique (LatticeSphere) et le census garde d'une boule certifiee (GuardedSphere),
// avec ou sans temoins sur la sphere (index.hpp : un noeud qui contient un temoin est raffine sans bornes evaluees).
#include "index/access.hpp"
#include "index/bounds.hpp"
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

template <class Bounds>
struct BorrowedPass {
  std::span<SiteIdx> storage;
  u32 threshold;
  std::span<const SiteIdx> witnesses;  // sites sur la sphere : leurs noeuds sont raffines sans bornes evaluees
  CensusLedger ledger;
  u64 p = 0, m = 0;

  bool holds_witness(const index_detail::Node& node) const noexcept {
    for (const SiteIdx w : witnesses)
      if (idx(w) >= node.begin && idx(w) < node.end) return true;
    return false;
  }

  Outcome inside(u32 begin, u32 end) noexcept {
    const u64 count = std::min<u64>(end - begin, threshold - p);
    if (p > storage.size() || m > storage.size() - p || count > storage.size() - p - m)
      return fail(Reason::arithmetic_invariant);
    for (u64 i = 0; i < count; ++i) storage[p + i] = SiteIdx{static_cast<u32>(begin + i)};
    p += count;
    return {};
  }
  Outcome points(const Cloud& cloud, u32 begin, u32 end, const Bounds& lattice) noexcept {
    const u32 *x = cloud.x().data(), *y = cloud.y().data(), *z = cloud.z().data();
    for (u32 i = begin; i < end && p < threshold; ++i) {
      ++ledger.point_tests;
      int side = 0;
      MHGP12_TRY(lattice.side_at(x[i], y[i], z[i], side, ledger));
      if (side < 0) { MHGP12_TRY(inside(i, i + 1)); }
      else if (side == 0) {
        if (p > storage.size() || m >= storage.size() - p) return fail(Reason::arithmetic_invariant);
        storage[storage.size() - 1 - m] = SiteIdx{i}; ++m;
      }
    }
    return {};
  }
  // Une preparation des bornes par parcours (lattice), sites de l'index entiers.
  Outcome walk(const GlobalIndex& index, const Bounds& lattice) noexcept {
    ++ledger.passes;
    const auto nodes = index_detail::Access::nodes(index);
    for (u64 cursor = 0; cursor < nodes.size() && p < threshold;) {
      const auto& node = nodes[cursor];
      ++ledger.nodes; ++ledger.bounds;
      if (holds_witness(node)) {  // minorant <= 0 et majorant >= 0 au temoin : ni exterieur ni interieur
        ++ledger.guard_witness;
        if (node.end - node.begin <= index.leaf_size()) {
          MHGP12_TRY(points(index.cloud(), node.begin, node.end, lattice)); cursor = node.escape;
        } else { ++cursor; }
        continue;
      }
      num::PowerBoundSigns signs;
      MHGP12_TRY(lattice.bound_signs(node.box, signs, ledger));
      if (signs.lower > 0) {
        ++ledger.outside_blocks; cursor = node.escape;
      } else if (signs.upper < 0) {
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

template <class Bounds, class Ball>
Outcome CensusWorkspace::run(const GlobalIndex& index, const Ball& sphere, u32 threshold,
                             std::span<const SiteIdx> witnesses, void* context, Callback callback) noexcept {
  if (active_.test_and_set(std::memory_order_acquire)) return fail(Reason::parameter_out_of_range);
  const ActiveQuery active(active_);
  if (&index != index_ || index.cloud().sites() != storage_.size() || threshold == 0 || callback == nullptr ||
      witnesses.size() > kMaxWitnesses)
    return fail(Reason::parameter_out_of_range);
  for (const SiteIdx w : witnesses)
    if (idx(w) >= storage_.size()) return fail(Reason::parameter_out_of_range);
  BorrowedPass<Bounds> pass{storage_.span(), threshold, witnesses, {}};
  {
    const Bounds lattice(sphere);
    MHGP12_TRY(pass.walk(index, lattice));
    lattice.flush(pass.ledger);
  }
  const bool saturated = pass.p == threshold;
  auto shell = storage_.span().last(saturated ? 0 : pass.m);
  std::reverse(shell.begin(), shell.end());
  const BorrowedCensus result(saturated ? CensusKind::saturated : CensusKind::complete,
                              storage_.span().first(pass.p), shell, pass.ledger);
  try { return callback(context, result); }
  catch (const std::bad_alloc&) { return fail(Reason::memory_budget); }
  catch (...) { return fail(Reason::task_exception); }
}

Outcome CensusWorkspace::query(const GlobalIndex& index, const num::Sphere& sphere, u32 threshold,
                               void* context, Callback callback) noexcept {
  return run<index_detail::GenericBounds>(index, sphere, threshold, {}, context, callback);
}

Outcome CensusWorkspace::query(const GlobalIndex& index, const num::CertifiedBall& ball, u32 threshold,
                               void* context, Callback callback) noexcept {
  return run<index_detail::GuardedBounds>(index, ball, threshold, {}, context, callback);
}

Outcome CensusWorkspace::query(const GlobalIndex& index, const num::CertifiedBall& ball, u32 threshold,
                               std::span<const SiteIdx> witnesses, void* context, Callback callback) noexcept {
  return run<index_detail::GuardedBounds>(index, ball, threshold, witnesses, context, callback);
}
}  // namespace mhgp12
