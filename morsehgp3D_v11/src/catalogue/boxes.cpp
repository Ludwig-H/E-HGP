// Boites T0 et listes K-certifiees : preparation possedee puis reprise, sans refiltrer un noeud prepare.
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

// Termes du test G1 separes par site : 2*largeur*(x-lo) par axe et |x-lo|^2. Leur difference redonne
// exactement 2*largeur*(x-y) ; memes entiers (<2^(2B+2)), donc memes decisions G1.
struct Terms {
  std::array<i64, 3> scaled;
  i64 square;
};

Terms terms(const std::array<i64, 3>& site, const Box& box) noexcept {
  Terms t{};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 x = site[axis] - box.lo[axis];
    t.scaled[axis] = 2 * (box.hi[axis] - box.lo[axis]) * x;
    t.square += x * x;
  }
  return t;
}

Outcome filter(Run& run, std::span<const SiteIdx> parent, const Box& box, Buffer<SiteIdx>& storage,
               u32& count) noexcept {
  MHGP11_TRY(storage.allocate(parent.size(), run.budget));
  std::array<SiteIdx, 3 * kMaxOrder> witnesses{};
  const u32 selected = reservoir(run.cloud, parent, box, run.params.kmax, witnesses);
  // Temoins pretraites une fois par noeud ; le compte de tests (meme arret a K dominateurs) est ajoute en fin.
  std::array<Terms, 3 * kMaxOrder> prepared{};
  for (u32 i = 0; i < selected; ++i) prepared[i] = terms(coordinates(run.cloud, witnesses[i]), box);
  const u32 kmax = static_cast<u32>(run.params.kmax);
  u64 tests = 0;  // <= |parent|*3K < 2^38
  count = 0;
  for (SiteIdx s : parent) {
    const Terms x = terms(coordinates(run.cloud, s), box);
    u32 found = 0, i = 0;
    for (; i < selected && found < kmax; ++i) {
      const auto& y = prepared[i];
      const i64 right = std::max<i64>(0, x.scaled[0] - y.scaled[0]) + std::max<i64>(0, x.scaled[1] - y.scaled[1]) +
                        std::max<i64>(0, x.scaled[2] - y.scaled[2]);
      found += x.square - y.square > right ? 1u : 0u;  // G1 : egalite conservee sur la fermeture de la boite.
    }
    tests += i;
    if (found < kmax) storage[count++] = s;
  }
  return checked_add(run.ledger.filter_tests, tests);
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
  ReadyNode ready;
  MHGP11_TRY(prepare_node(run, parent, box, depth, ready));
  return ready.count == 0 ? Outcome{} : run_ready(run, ready);
}

}  // namespace

Outcome NodeQuota::claim() noexcept {
  if (limit_ == 0) return {};
  u64 before = claimed_.load(std::memory_order_relaxed);
  while (before < limit_) {
    if (claimed_.compare_exchange_weak(before, before + 1, std::memory_order_relaxed)) return {};
  }
  return fail(Reason::node_budget);
}

Outcome prepare_node(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth,
                     ReadyNode& ready) noexcept {
  ready = ReadyNode{};
  if (depth > kMaxDepth) return fail(Reason::catalogue_invariant);
  if (run.quota != nullptr) {
    if (run.quota->limit() != run.params.max_nodes) return fail(Reason::catalogue_invariant);
    MHGP11_TRY(run.quota->claim());
  }
  MHGP11_TRY(checked_add(run.ledger.nodes, 1));
  if (run.quota == nullptr && run.params.max_nodes != 0 && run.ledger.nodes > run.params.max_nodes)
    return fail(Reason::node_budget);
  run.ledger.max_depth = std::max<u64>(run.ledger.max_depth, depth);
  u32 count = 0;
  MHGP11_TRY(filter(run, parent, box, ready.storage, count));
  if (count == 0) return {};
  const auto sites = ready.storage.span().first(count);
  Box adjusted = envelope(run.cloud, sites);
  for (int i = 0; i < 3; ++i) {
    adjusted.lo[i] = std::max(adjusted.lo[i], box.lo[i]);
    adjusted.hi[i] = std::min(adjusted.hi[i], box.hi[i]);
    if (adjusted.lo[i] >= adjusted.hi[i]) return {};
  }
  ready.count = count;
  ready.depth = depth;
  ready.box = adjusted;
  return {};
}

bool split_ready(const ReadyNode& ready, const CatalogueParams& params, Box& left, Box& right) noexcept {
  int axis = 0;
  for (int i = 1; i < 3; ++i)
    if (ready.box.hi[i] - ready.box.lo[i] > ready.box.hi[axis] - ready.box.lo[axis]) axis = i;
  const i64 width = ready.box.hi[axis] - ready.box.lo[axis];
  if (ready.count <= params.leaf_size || width <= 1) return false;
  const i64 middle = ready.box.lo[axis] + width / 2;
  left = ready.box;
  right = ready.box;
  left.hi[axis] = middle;
  right.lo[axis] = middle;
  return true;
}

Outcome run_ready(Run& run, const ReadyNode& ready) noexcept {
  if (ready.count == 0 || ready.count > ready.storage.size() || ready.depth > kMaxDepth)
    return fail(Reason::catalogue_invariant);
  Box left, right;
  if (split_ready(ready, run.params, left, right)) {
    MHGP11_TRY(process(run, ready.sites(), left, ready.depth + 1));
    return process(run, ready.sites(), right, ready.depth + 1);
  }
  MHGP11_TRY(checked_add(run.ledger.leaves, 1));
  run.ledger.max_leaf = std::max<u64>(run.ledger.max_leaf, ready.count);
  if (ready.count > run.params.max_leaf) return fail(Reason::wide_leaf);
  return enumerate_leaf(run, ready.sites(), ready.box);
}

Outcome make_root(Run& run, Buffer<SiteIdx>& root, Box& box) noexcept {
  if (run.cloud.sites() == 0) return fail(Reason::empty_input);
  MHGP11_TRY(root.allocate(run.cloud.sites(), run.budget));
  for (u32 i = 0; i < run.cloud.sites(); ++i) root[i] = make_id<SiteIdx>(i);
  box = envelope(run.cloud, root.span());
  return {};
}

Outcome walk(Run& run) noexcept {
  Buffer<SiteIdx> root;
  Box box;
  MHGP11_TRY(make_root(run, root, box));
  // Racine rectangulaire ; somme ceil(log2 largeur)<=3B. Chaque coupe diminue ce potentiel d'au moins un,
  // les ajustements ne l'augmentent pas. Au plus 3B+1 listes filtrees simultanees, plus la liste racine.
  return process(run, root.span(), box, 0);
}

}  // namespace mhgp11::catalogue_detail
