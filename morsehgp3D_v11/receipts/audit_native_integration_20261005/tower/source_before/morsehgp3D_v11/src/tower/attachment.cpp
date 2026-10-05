// Rattachement exact de W_K sans descente (lemme D) : les graines du journal sont relevees a la coupe ouverte
// r_b - 1 par un seul balayage ferme croissant, puis att(b) suit la regle du parent ; roles par les rangs (lemme B),
// branches dedupliquees a la coupe stricte (lemme C). Controles I1 a I4 dans le produit : tout ecart rend
// tower_invariant, sans resultat.
#include <algorithm>
#include "tower/forest_ancestor_sweep.hpp"
#include "tower/order_tree.hpp"
#include "tower/seed_log.hpp"

namespace mhgp11::tower_detail {

struct AttachmentBuilder {
  const FullDomain& domain;
  const OrderForest& forest;
  SeedLog& log;
  MemoryBudget& budget;
  WindowAttachment result{};
  Buffer<u32> stamp;  // noeud -> rang du dernier plateau qui l'a touche (kNone : jamais)
  u64 position = 0;   // curseur de W_K, cellules du journal en BallIdx croissants
  // Registres recomptes depuis le rattachement (I3, I4) et branches compactees dans le journal.
  u64 births = 0, traces = 0, combinations = 0, plateaus = 0, touched = 0, continuations = 0, merged = 0;
  u64 prior_total = 0;

  AttachmentBuilder(const FullDomain& d, const OrderForest& f, SeedLog& l, MemoryBudget& b) noexcept
      : domain(d), forest(f), log(l), budget(b) {}

  // I1 : W_K recalcule depuis le catalogue par la fenetre, BallIdx croissants ; resultat et marques admis ici.
  Outcome window() noexcept {
    const auto balls = domain.catalogue().balls_data();
    const u32 k = forest.order();
    u64 count = 0;
    for (const auto& ball : balls) count += in_window(ball, k) ? 1 : 0;
    const u64 nodes = forest.nodes().size();
    // count <= B < 2^32 et nodes < 2^32 : somme < 2^38.
    const u64 bytes = count * (sizeof(BallIdx) + sizeof(NodeIdx) + sizeof(BallRole) + 2 * sizeof(u32)) +
                      (count + 1) * sizeof(u64) + nodes * sizeof(u32);
    MHGP11_TRY(budget.admit(bytes));
    MHGP11_TRY(result.balls_.allocate(count, budget));
    MHGP11_TRY(result.node_.allocate(count, budget));
    MHGP11_TRY(result.role_.allocate(count, budget));
    MHGP11_TRY(result.strict_.allocate(count, budget));
    MHGP11_TRY(result.components_.allocate(count, budget));
    MHGP11_TRY(result.prior_offsets_.allocate(count + 1, budget));
    MHGP11_TRY(stamp.allocate(nodes, budget));
    u64 at = 0;
    for (u32 b = 0; b < balls.size(); ++b) {
      if (!in_window(balls[b], k)) continue;
      result.balls_[at] = BallIdx{b};
      result.node_[at] = NodeIdx{kNone};
      result.role_[at] = BallRole::internal;
      result.strict_[at] = 0;
      result.components_[at++] = 0;
    }
    std::fill(stamp.begin(), stamp.end(), kNone);
    return {};
  }

  // Lemme B, naissances (K >= 2) : chaque naissance de la foret est une boule de W_K, de meme rang, rattachee a son
  // propre noeud ; elle est forte (p+q <= K), donc une boule faible (K = p+q-1) n'est jamais une naissance. A K = 1 les
  // naissances sont les sites : aucune boule n'en est une.
  Outcome birth_nodes() noexcept {
    if (forest.order() == 1) return {};
    const auto balls = domain.catalogue().balls_data();
    const auto window = result.balls_.span();
    for (u32 v = 0; v < forest.births(); ++v) {
      const auto& node = forest.nodes()[v];
      const auto found = std::lower_bound(window.begin(), window.end(), BallIdx{node.birth_key},
                                          [](BallIdx a, BallIdx b) noexcept { return idx(a) < idx(b); });
      if (found == window.end() || idx(*found) != node.birth_key) return fail(Reason::tower_invariant);
      const u64 at = static_cast<u64>(found - window.begin());
      const auto& data = balls[node.birth_key];
      if (result.node_[at] != NodeIdx{kNone} || data.rank != node.rank || u64{data.p} + data.qmin > forest.order())
        return fail(Reason::tower_invariant);
      result.node_[at] = NodeIdx{v};
      result.role_[at] = BallRole::birth;
      ++births;
    }
    return {};
  }

  // Une cellule du journal au rang r, balayage deja avance a r - 1 (coupe ouverte). Graines -> noeuds de coupe
  // ouverte (u), dedupliques : ant(b). Regle du parent : a(u) = parent(u) si ce parent a le rang r, sinon u ; une
  // seule valeur pour toutes les traces (T3). Role par les rangs ; branches publiees pour le role fusion seulement.
  Outcome cell(u32 c, LevelRank rank, ClosedAncestorSweep& sweep, ForestLedger& work) noexcept {
    const BallIdx ball = log.ball(c);
    while (position < result.balls_.size() && idx(result.balls_[position]) < idx(ball)) ++position;
    if (position == result.balls_.size() || result.balls_[position] != ball ||
        result.node_[position] != NodeIdx{kNone}) return fail(Reason::tower_invariant);
    const u64 begin = log.begin(c), end = log.end(c);
    if (begin >= end || end > log.storage().size()) return fail(Reason::tower_invariant);
    auto strict = published_traces(end - begin);  // avant toute conversion en u32
    if (!strict.ok()) return strict.outcome();
    const auto nodes = forest.nodes();
    auto seeds = log.storage().subspan(begin, end - begin);
    for (NodeIdx& seed : seeds) {
      if (idx(seed) >= forest.births() || idx(nodes[idx(seed)].rank) >= idx(rank)) return fail(Reason::tower_invariant);
      auto open = sweep.query(seed, work);
      if (!open.ok()) return open.outcome();
      seed = open.value();
    }
    std::sort(seeds.begin(), seeds.end(), [](NodeIdx a, NodeIdx b) noexcept { return idx(a) < idx(b); });
    const u64 branches = static_cast<u64>(std::unique(seeds.begin(), seeds.end()) - seeds.begin());
    NodeIdx att{kNone};
    for (NodeIdx u : seeds.first(branches)) {
      const NodeIdx parent = nodes[idx(u)].parent;
      const NodeIdx a = parent != NodeIdx{kNone} && nodes[idx(parent)].rank == rank ? parent : u;
      if (att != NodeIdx{kNone} && a != att) return fail(Reason::tower_invariant);
      att = a;
      if (stamp[idx(u)] == idx(rank)) continue;  // deja touche a ce plateau
      stamp[idx(u)] = idx(rank);
      ++touched;
      if (a == u) ++continuations; else ++merged;
    }
    const BallRole role = nodes[idx(att)].rank == rank ? BallRole::merge : BallRole::internal;
    MHGP11_TRY(check_roles(att, role, rank, branches));
    const auto& data = domain.catalogue().balls_data()[idx(ball)];
    auto universe = cell_binomial(data.m, forest.order() - data.p);
    if (!universe.ok()) return universe.outcome();
    MHGP11_TRY(cell_add(combinations, universe.value()));
    MHGP11_TRY(cell_add(traces, end - begin));
    result.node_[position] = att;
    result.role_[position] = role;
    result.strict_[position] = strict.value();
    result.components_[position] = static_cast<u32>(branches);  // branches <= traces <= UINT32_MAX
    if (role == BallRole::merge) {
      // Branches compactees en tete du journal : prior_total <= begin (une branche par graine au plus).
      if (prior_total != begin) std::copy(seeds.begin(), seeds.begin() + branches, log.storage().begin() + prior_total);
      prior_total += branches;
    }
    return {};
  }

  // Lemmes B et C : fusion creee a ce plateau, ou continuation d'une seule branche vivante a la coupe fermee.
  Outcome check_roles(NodeIdx att, BallRole role, LevelRank rank, u64 branches) const noexcept {
    const auto nodes = forest.nodes();
    const ForestNode& node = nodes[idx(att)];
    if (role == BallRole::merge)
      return idx(att) >= forest.births() && node.rank == rank ? Outcome{} : fail(Reason::tower_invariant);
    const bool alive = node.parent == NodeIdx{kNone} || idx(nodes[idx(node.parent)].rank) > idx(rank);
    return branches == 1 && idx(node.rank) < idx(rank) && alive ? Outcome{} : fail(Reason::tower_invariant);
  }

  // Groupes du journal de meme rang (strictement croissants) : un seul advance(r - 1) par plateau, monotone.
  Outcome cells() noexcept {
    auto sweep = ClosedAncestorSweep::make(forest, budget);
    if (!sweep.ok()) return sweep.outcome();
    ForestLedger work;
    const auto balls = domain.catalogue().balls_data();
    std::optional<LevelRank> previous;
    for (u32 c = 0; c < log.cells();) {
      const LevelRank rank = balls[idx(log.ball(c))].rank;
      if (idx(rank) == 0 || (previous && idx(rank) <= idx(*previous))) return fail(Reason::tower_invariant);
      MHGP11_TRY(sweep.value().advance(LevelRank{idx(rank) - 1}, work));
      ++plateaus;
      for (; c < log.cells() && balls[idx(log.ball(c))].rank == rank; ++c)
        MHGP11_TRY(cell(c, rank, sweep.value(), work));
      previous = rank;
    }
    return {};
  }

  // I1 (chaque boule de W_K une fois, naissances), I3 (toute fusion recoit toutes ses branches par ses boules de role
  // fusion) et I4 (registres de la foret calcules par une autre voie : DSU, cellules, descentes).
  Outcome registers() const noexcept {
    for (NodeIdx node : result.node_.span()) if (node == NodeIdx{kNone}) return fail(Reason::tower_invariant);
    const auto& ledger = forest.ledger();
    const bool ok = births == (forest.order() == 1 ? 0 : forest.births()) && merged == forest.edges().size() &&
                    traces == ledger.trace_resolutions && combinations == ledger.cells.combinations &&
                    log.cells() == ledger.replayed_cells && plateaus == ledger.plateaus &&
                    touched == ledger.touched_components && continuations == ledger.continuations;
    return ok ? Outcome{} : fail(Reason::tower_invariant);
  }

  // Branches du role fusion, dans l'ordre de W_K : tableau exact, decalages par sommes prefixes.
  Outcome publish_prior() noexcept {
    stamp.reset();
    MHGP11_TRY(budget.admit(prior_total * sizeof(NodeIdx)));
    MHGP11_TRY(result.prior_.allocate(prior_total, budget));
    std::copy_n(log.storage().begin(), prior_total, result.prior_.begin());
    u64 offset = 0;
    for (u64 at = 0; at < result.balls_.size(); ++at) {
      result.prior_offsets_[at] = offset;
      if (result.role_[at] == BallRole::merge) offset += result.components_[at];
    }
    result.prior_offsets_[result.balls_.size()] = offset;
    return offset == prior_total ? Outcome{} : fail(Reason::tower_invariant);
  }
};

Result<WindowAttachment> attach_window(const FullDomain& domain, const OrderForest& forest, SeedLog& log,
                                       MemoryBudget& budget) noexcept {
  if (forest.nodes().empty() || forest.order() == 0 || forest.order() > domain.catalogue().kmax())
    return fail(Reason::tower_invariant);
  AttachmentBuilder builder(domain, forest, log, budget);
  MHGP11_TRY(builder.window());
  MHGP11_TRY(builder.birth_nodes());
  MHGP11_TRY(builder.cells());
  MHGP11_TRY(builder.registers());
  MHGP11_TRY(builder.publish_prior());
  return std::move(builder.result);
}

}  // namespace mhgp11::tower_detail
