// Boites T0 et listes K-certifiees : port des lemmes de generator.cpp R2, sans frontiere parallele ni flottant.
#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {
namespace {

// x-lo dans (-M,M), largeur <=M, M=2^B. Carres <3M^2, termes de dominance <12M^2,
// cles du reservoir <12M^2. La marge 2B+5 couvre chaque somme partielle en i64 aux trois profils.
static_assert(2 * kCoordBits + 5 <= 63, "catalogue : dominance T0 et reservoir en i64");

std::array<i64, 3> coordinates(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  return {cloud.x()[i], cloud.y()[i], cloud.z()[i]};
}

bool dominates(const std::array<i64, 3>& witness, const std::array<i64, 3>& candidate,
               const Box& box) noexcept {
  i64 a = 0, b = 0, right = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 x = candidate[axis] - box.lo[axis], y = witness[axis] - box.lo[axis];
    const i64 width = box.hi[axis] - box.lo[axis];
    a += x * x;
    b += y * y;
    right += std::max<i64>(0, 2 * width * (x - y));
  }
  return a - b > right;  // G1 : egalite conservee sur la fermeture de la boite.
}

u32 reservoir(const Cloud& cloud, std::span<const SiteIdx> parent, const Box& box, int kmax,
              std::array<SiteIdx, 3 * kMaxOrder>& sites) noexcept {
  std::array<i64, 3 * kMaxOrder> distances{};
  const u32 cap = static_cast<u32>(std::min<u64>(parent.size(), 3u * static_cast<u32>(kmax)));
  u32 count = 0;
  for (SiteIdx s : parent) {
    const auto x = coordinates(cloud, s);
    i64 distance = 0;
    for (int axis = 0; axis < 3; ++axis) {
      const i64 delta = 2 * x[axis] - box.lo[axis] - box.hi[axis];
      distance += delta * delta;
    }
    if (count == cap && distance >= distances[count - 1]) continue;
    u32 at = count < cap ? count++ : count - 1;
    while (at > 0 && distances[at - 1] > distance) {
      distances[at] = distances[at - 1];
      sites[at] = sites[at - 1];
      --at;
    }
    distances[at] = distance;
    sites[at] = s;
  }
  return count;
}

Outcome filter(Run& run, std::span<const SiteIdx> parent, const Box& box, Buffer<SiteIdx>& storage,
               u32& count) noexcept {
  MHGP11_TRY(storage.allocate(parent.size(), run.budget));
  std::array<SiteIdx, 3 * kMaxOrder> witnesses{};
  const u32 selected = reservoir(run.cloud, parent, box, run.params.kmax, witnesses);
  count = 0;
  for (SiteIdx s : parent) {
    u32 found = 0;
    const auto x = coordinates(run.cloud, s);
    for (u32 i = 0; i < selected && found < static_cast<u32>(run.params.kmax); ++i) {
      MHGP11_TRY(checked_add(run.ledger.filter_tests, 1));
      found += dominates(coordinates(run.cloud, witnesses[i]), x, box) ? 1u : 0u;
    }
    if (found < static_cast<u32>(run.params.kmax)) storage[count++] = s;
  }
  return {};
}

Box envelope(const Cloud& cloud, std::span<const SiteIdx> sites) noexcept {
  const auto initial = coordinates(cloud, sites.front());
  Box box{initial, initial};
  for (SiteIdx s : sites) {
    const auto x = coordinates(cloud, s);
    for (int axis = 0; axis < 3; ++axis) {
      box.lo[axis] = std::min(box.lo[axis], x[axis]);
      box.hi[axis] = std::max(box.hi[axis], x[axis]);
    }
  }
  for (i64& high : box.hi) ++high;  // 2^B representable ; garde le centre au maximum exact.
  return box;
}

Outcome process(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth) noexcept {
  if (depth > kMaxDepth) return fail(Reason::catalogue_invariant);
  MHGP11_TRY(checked_add(run.ledger.nodes, 1));
  if (run.params.max_nodes != 0 && run.ledger.nodes > run.params.max_nodes) return fail(Reason::node_budget);
  run.ledger.max_depth = std::max<u64>(run.ledger.max_depth, depth);
  Buffer<SiteIdx> storage;
  u32 count = 0;
  MHGP11_TRY(filter(run, parent, box, storage, count));
  if (count == 0) return {};
  const auto sites = storage.span().first(count);
  Box adjusted = envelope(run.cloud, sites);
  int axis = 0;
  for (int i = 0; i < 3; ++i) {
    adjusted.lo[i] = std::max(adjusted.lo[i], box.lo[i]);
    adjusted.hi[i] = std::min(adjusted.hi[i], box.hi[i]);
    if (adjusted.lo[i] >= adjusted.hi[i]) return {};
    if (adjusted.hi[i] - adjusted.lo[i] > adjusted.hi[axis] - adjusted.lo[axis]) axis = i;
  }
  const i64 width = adjusted.hi[axis] - adjusted.lo[axis];
  if (count > run.params.leaf_size && width > 1) {
    const i64 middle = adjusted.lo[axis] + width / 2;
    Box left = adjusted, right = adjusted;
    left.hi[axis] = middle;
    right.lo[axis] = middle;
    MHGP11_TRY(process(run, sites, left, depth + 1));
    return process(run, sites, right, depth + 1);
  }
  MHGP11_TRY(checked_add(run.ledger.leaves, 1));
  run.ledger.max_leaf = std::max<u64>(run.ledger.max_leaf, count);
  if (count > run.params.max_leaf) return fail(Reason::wide_leaf);
  return enumerate_leaf(run, sites, adjusted);
}

}  // namespace

Outcome walk(Run& run) noexcept {
  Buffer<SiteIdx> root;
  MHGP11_TRY(root.allocate(run.cloud.sites(), run.budget));
  for (u32 i = 0; i < run.cloud.sites(); ++i) root[i] = make_id<SiteIdx>(i);
  // Racine rectangulaire ; somme ceil(log2 largeur)<=3B. Chaque coupe diminue ce potentiel d'au moins un,
  // les ajustements ne l'augmentent pas. Au plus 3B+1 listes filtrees simultanees, plus la liste racine.
  const Box box = envelope(run.cloud, root.span());
  return process(run, root.span(), box, 0);
}

}  // namespace mhgp11::catalogue_detail
