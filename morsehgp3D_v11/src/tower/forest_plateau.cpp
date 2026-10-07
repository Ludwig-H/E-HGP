// Unions par plateau sans noeud binaire intermediaire ; listes des seules anciennes composantes touchees.
#include "tower/forest_internal.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/regular_vertical_seeds.hpp"
#include "tower/seed_log.hpp"

namespace mhgp11::tower_detail {

u32 ForestBuilder::find(u32 start) noexcept {
  u32 root = start;
  while (parents[root] != root) root = parents[root];
  while (parents[start] != start) {
    const u32 next = parents[start]; parents[start] = root; start = next;
  }
  return root;
}

Outcome ForestBuilder::touch(u32 root) noexcept {
  auto& state = states[root];
  if (state.touched) return {};
  if (touched_count >= result.births_) return fail(Reason::tower_invariant);
  touched[touched_count++] = root;
  state.touched = true; state.head = root; state.tail = root; state.next = kNone;
  // Noeud courant de la composante : close le relira et le rattachera a la fin du plateau, quelques cellules plus
  // loin ; lecture au hasard dans un tableau de ~17 Mo a K5, prechargee des maintenant, en ecriture.
  if (state.top < result.nodes_.size()) __builtin_prefetch(&result.nodes_[state.top], 1);
  return cell_add(result.ledger_.touched_components, 1);
}

// Deux racines deja trouvees et touchees (cell, regular_cell) : fusion sans nouvelle recherche ; root devient
// la racine commune, toujours touchee. Remplace unite(a,b), dont les find/touch repetes etaient sans effet.
Outcome ForestBuilder::unite_roots(u32& root, u32 b) noexcept {
  u32 a = root;
  if (a == b) return {};
  if (b < a) std::swap(a, b);  // Racine = plus petite naissance canonique de la composante.
  parents[b] = a;
  states[states[a].tail].next = states[b].head;
  states[a].tail = states[b].tail;
  root = a;
  return cell_add(result.ledger_.unions, 1);
}

Outcome ForestBuilder::cell(BallIdx ball) noexcept {
  auto made = build_cell(domain, ball, static_cast<Order>(k), budget);
  if (!made.ok()) return made.outcome();
  if (made.value().kind() != CellKind::strict_traces) return fail(Reason::tower_invariant);
  MHGP11_TRY(cell_add(result.ledger_.replayed_cells, 1));
  MHGP11_TRY(add_cell_work(result.ledger_.cells, made.value().ledger()));
  const auto& data = domain.catalogue().balls_data()[idx(ball)];
  const auto& level = domain.catalogue().levels()[idx(data.rank)];
  std::optional<u32> first;
  std::optional<NodeIdx> representative;
  if (seed_log != nullptr) MHGP11_TRY(seed_log->open(ball));
  for (const auto& trace : made.value().traces()) {
    auto down = resolve_descent(domain, trace.part(), k, budget, memo, extended_scratch, population);
    if (!down.ok()) return down.outcome();
    // La DATE initiale garantit une composante preplateau ; le niveau terminal seul ne suffit pas.
    if (num::compare(down.value().initial_level(), level) >= 0) return fail(Reason::tower_invariant);
    MHGP11_TRY(cell_add(result.ledger_.trace_resolutions, 1));
    MHGP11_TRY(add_descent(result.ledger_.descent, down.value().ledger()));
    const auto seed = result.birth_node(down.value().seed());
    if (!seed || idx(result.nodes_[idx(*seed)].rank) >= idx(data.rank)) return fail(Reason::tower_invariant);
    if (seed_log != nullptr) MHGP11_TRY(seed_log->add(*seed));  // la naissance rendue, jamais sa racine
    if (vertical_seeds != nullptr && !representative) representative = *seed;
    const u32 root = find(idx(*seed));
    MHGP11_TRY(touch(root));
    // first reste la racine courante de la composante reunie (touchee) : aucune recherche repetee.
    if (first) MHGP11_TRY(unite_roots(*first, root)); else first = root;
  }
  if (vertical_seeds != nullptr && representative)
    MHGP11_TRY(vertical_seeds->remember(result, ball, *representative));
  return {};
}

Outcome ForestBuilder::close(LevelRank level) noexcept {
  forest_sort(touched.span().first(touched_count), [](u32 a, u32 b) noexcept { return a < b; });
  for (u32 i = 0; i < touched_count; ++i) {
    const u32 root = touched[i];
    if (parents[root] != root) continue;
    u64 count = 0;
    // Une chaine plus longue que les naissances est cyclique : refus par code plutot que boucle sans fin.
    for (u32 r = states[root].head; r != kNone; r = states[r].next)
      if (++count > result.births_) return fail(Reason::tower_invariant);
    if (count < 2) { MHGP11_TRY(cell_add(result.ledger_.continuations, 1)); continue; }
    if (result.count_ >= result.nodes_.size() || count > result.children_.size() - result.edges_)
      return fail(Reason::tower_invariant);
    const NodeIdx node{result.count_};
    const u64 begin = result.edges_;
    for (u32 r = states[root].head; r != kNone; r = states[r].next) {
      const NodeIdx child{states[r].top};
      if (idx(child) >= result.count_ || idx(result.nodes_[idx(child)].rank) >= idx(level) ||
          result.nodes_[idx(child)].parent != NodeIdx{kNone}) return fail(Reason::tower_invariant);
      result.children_[result.edges_++] = child;
      result.nodes_[idx(child)].parent = node;
    }
    forest_sort(result.children_.span().subspan(begin, count),
                [](NodeIdx a, NodeIdx b) noexcept { return idx(a) < idx(b); });
    result.nodes_[result.count_++] = {level, NodeIdx{kNone}, begin, static_cast<u32>(count), kNone};
    states[root].top = idx(node);  // Tous les anciens top ont ete consommes ; aucun nouveau top n'est dans la chaine.
  }
  for (u32 i = 0; i < touched_count; ++i) states[touched[i]].touched = false;
  touched_count = 0;
  return {};
}

Outcome ForestBuilder::regular_cell(BallIdx ball, std::span<const NodeIdx> seeds) noexcept {
  const auto& data = domain.catalogue().balls_data()[idx(ball)];
  if (data.m != data.qmin || seeds.size() != data.qmin || k != u64{data.p} + data.qmin - 1)
    return fail(Reason::tower_invariant);
  MHGP11_TRY(cell_add(result.ledger_.replayed_cells, 1));
  MHGP11_TRY(cell_add(result.ledger_.cells.combinations, data.qmin));
  MHGP11_TRY(cell_add(result.ledger_.trace_resolutions, data.qmin));
  if (seed_log != nullptr) MHGP11_TRY(seed_log->open(ball));
  std::optional<u32> first;
  for (NodeIdx seed : seeds) {
    if (idx(seed) >= result.births_ || idx(result.nodes_[idx(seed)].rank) >= idx(data.rank))
      return fail(Reason::tower_invariant);
    if (seed_log != nullptr) MHGP11_TRY(seed_log->add(seed));  // graine reguliere, jamais sa racine
    const u32 root = find(idx(seed));
    MHGP11_TRY(touch(root));
    if (first) MHGP11_TRY(unite_roots(*first, root)); else first = root;  // racine courante, deja touchee
  }
  if (vertical_seeds != nullptr) {
    if (seeds.empty()) return fail(Reason::tower_invariant);
    MHGP11_TRY(vertical_seeds->remember(result, ball, seeds.front()));
  }
  return {};
}

Outcome ForestBuilder::plateaus() noexcept {
  if (parallel != nullptr) return parallel->run(*this);
  const auto balls = domain.catalogue().balls_data();
  u32 begin = 0;
  while (begin < balls.size()) {
    const LevelRank level = balls[begin].rank;
    u32 end = begin + 1;
    while (end < balls.size() && balls[end].rank == level) ++end;
    bool active = false;
    for (u32 b = begin; b < end; ++b) if (kinds[b] == 2) {
      active = true; MHGP11_TRY(cell(BallIdx{b}));
    }
    if (active) { MHGP11_TRY(cell_add(result.ledger_.plateaus, 1)); MHGP11_TRY(close(level)); }
    begin = end;
  }
  return {};
}

}  // namespace mhgp11::tower_detail
