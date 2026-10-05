// Incidences fortes par site (tranche S9) : port explicite du bloc d'incidences de MHGP11PH (bench/points_export.cpp,
// write_incidences et ball_nodes), avec le noeud de rattachement lu dans WindowAttachment au lieu d'une descente (porte
// mhgp11_tower_attach_export : memes octets). K = 1 : la feuille de chaque site, au rang 0 (bench/points_export.cpp,
// singletons). Deux passes (comptes, puis remplissage), aucun tableau par paire ; tri de chaque ligne par (rang,
// noeud) en parallele, a positions fixes.
#include "points/internal.hpp"

namespace mhgp11::points_detail {

namespace {

struct SortRows {
  Incidences* out = nullptr;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    Incidences& inc = *static_cast<SortRows*>(context)->out;
    for (u64 s = begin; s < end; ++s) {
      u64* first = inc.packed.data() + inc.offsets[s];
      std::sort(first, inc.packed.data() + inc.offsets[s + 1]);
    }
    return {};
  }
};

// Boule forte de W_K : p + q_min <= K (une boule faible, K = p + q_min - 1, n'a pas d'incidence forte).
bool strong(const CatalogueBall& ball, Order k) noexcept { return u64{ball.p} + ball.qmin <= k; }

// Feuille de chaque site a K = 1 : les naissances sont les sites (birth_key = SiteIdx), au rang 0.
Outcome leaves(const OrderForest& forest, u32 n, Incidences& out) noexcept {
  if (forest.births() != n) return fail(Reason::points_invariant);
  for (u32 s = 0; s < n; ++s) out.packed[s] = ~u64{0};
  for (u32 v = 0; v < n; ++v) {
    const ForestNode& node = forest.nodes()[v];
    if (node.birth_key >= n || idx(node.rank) != 0 || out.packed[node.birth_key] != ~u64{0})
      return fail(Reason::points_invariant);
    out.packed[node.birth_key] = pack(0, v);
  }
  return {};
}

}  // namespace

Outcome run(sched::Pool* pool, u64 n, void* context, sched::Pool::Body body) noexcept {
  if (pool == nullptr) return body(context, 0, n, 0);
  return pool->parallel_for(n, kGrain, context, body);
}

Outcome build_incidences(const OrderTree& tree, MemoryBudget& budget, sched::Pool* pool, Incidences& out) noexcept {
  const Catalogue& catalogue = tree.domain().catalogue();
  const OrderForest& forest = tree.forest();
  const WindowAttachment& attachment = tree.attachment();
  const auto balls = catalogue.balls_data();
  const Order k = forest.order();
  const u32 n = tree.domain().index().cloud().sites();
  const u64 nodes = forest.nodes().size();
  MHGP11_TRY(budget.admit(8 * (u64{n} + 1)));
  MHGP11_TRY(out.offsets.allocate(u64{n} + 1, budget));
  std::fill(out.offsets.begin(), out.offsets.end(), u64{0});
  if (k == 1) {
    for (u32 s = 0; s < n; ++s) out.offsets[s + 1] = 1;
  } else {
    for (u32 j = 0; j < attachment.size(); ++j) {
      const u32 b = idx(attachment.balls()[j]);
      if (b >= balls.size() || idx(attachment.node()[j]) >= nodes) return fail(Reason::points_invariant);
      if (!strong(balls[b], k)) continue;
      for (const auto part : {catalogue.interior(BallIdx{b}), catalogue.shell(BallIdx{b})}) {
        for (const SiteIdx site : part) {
          if (idx(site) >= n) return fail(Reason::points_invariant);
          out.offsets[idx(site) + 1] += 1;
        }
      }
    }
  }
  for (u32 s = 0; s < n; ++s) {
    out.widest = std::max(out.widest, out.offsets[s + 1]);
    out.offsets[s + 1] += out.offsets[s];
  }
  const u64 total = out.offsets[n];
  Buffer<u64> cursor;
  MHGP11_TRY(budget.admit(8 * total + 8 * u64{n}));
  MHGP11_TRY(out.packed.allocate(total, budget));
  if (k == 1) return leaves(forest, n, out);
  MHGP11_TRY(cursor.allocate(n, budget));
  std::copy_n(out.offsets.begin(), n, cursor.begin());
  for (u32 j = 0; j < attachment.size(); ++j) {
    const u32 b = idx(attachment.balls()[j]);
    if (!strong(balls[b], k)) continue;
    if (idx(balls[b].rank) == kNone) return fail(Reason::points_invariant);
    const u64 value = pack(idx(balls[b].rank), idx(attachment.node()[j]));
    for (const auto part : {catalogue.interior(BallIdx{b}), catalogue.shell(BallIdx{b})})
      for (const SiteIdx site : part) out.packed[cursor[idx(site)]++] = value;
  }
  for (u32 s = 0; s < n; ++s)
    if (cursor[s] != out.offsets[s + 1] || out.offsets[s + 1] == out.offsets[s]) return fail(Reason::points_invariant);
  cursor.reset();
  SortRows sort{&out};
  return run(pool, n, &sort, &SortRows::body);
}

}  // namespace mhgp11::points_detail
