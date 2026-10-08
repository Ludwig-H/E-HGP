// Etage T : noyau union-find par TAILLE sans lots d'un ordre (D-F1, LEM-T4 ; CONCEPTION_TOUR.md de la v11, paragraphe
// 4.1), port du noyau de MES-M4, en deux temps :
//   pre-passe (parallele par morceaux de cellules, hors du passage sequentiel) : la cible de chaque representant devient
//   sa feuille canonique (naissance, par son noeud) ou garde son codage << cellule >> ; la date de LEM-T3 est controlee
//   (rang de la cible strictement inferieur a celui de la cellule, donc cellule cible deja traitee) ;
//   noyau (sequentiel, un proprietaire par ordre) : cellules par rang croissant ; feuille de chaque representant (une
//   cible << cellule >> se lit comme l'element de la cellule deja traitee), relue a sa racine courante ; chaque union de
//   deux racines distinctes est un evenement binaire (rang, sommets des deux composantes, plus petite feuille,
//   survivant), avec sa cellule ; attache du perdant avec son rang ; sommet et element de la cellule apres traitement.
// Aucune allocation dans la boucle. Les plateaux sont rendus atomiques par la contraction (LEM-T4) : l'ordre des unions
// a rang egal et la regle d'union n'y changent rien ; l'union par taille borne l'historique d'attache (LEM-T5 (i)).
#include <algorithm>

#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {
namespace {

struct KernelView {
  UnionCell* cells;
  Event* events;
  u32* event_cell;
  u32* attach_parent;
  u32* attach_rank;
  u32* element;  // par cellule traitee : feuille de sa composante apres traitement
};

inline u32 find(UnionCell* c, u32 x) noexcept {
  while (c[x].up != x) {
    c[x].up = c[c[x].up].up;  // demi-compression
    x = c[x].up;
  }
  return x;
}

inline u32 top(const UnionCell* c, u32 x) noexcept { return c[x].last == kNone ? x : (c[x].last | kEventBit); }

// Feuille d'un representant : sa naissance canonique, ou l'element de la cellule cible (deja traitee).
inline u32 leaf_of(const KernelView& v, u32 code) noexcept {
  return target_is_cell(code) ? v.element[target_index(code)] : code;
}

// Une cellule : unions de ses representants ; faux si une union deborderait (au plus naissances - 1 : jamais dans
// le domaine).
bool process_cell(const ForestInput& in, const u32* leaves, u32 nb, KernelView& v, u32 t, u32& count,
                  u32& top_out) noexcept {
  const u64 b0 = in.rep_offsets[t], e0 = in.rep_offsets[t + 1], nr = in.targets.size();
  const u32 r = idx(in.cell_rank[t]);
  UnionCell* c = v.cells;
  for (u64 p = b0 + 16; p < e0 + 16 && p < nr; ++p)
    if (!target_is_cell(leaves[p])) __builtin_prefetch(&c[leaves[p]]);
  u32 x = find(c, leaf_of(v, leaves[b0]));
  for (u64 p = b0 + 1; p < e0; ++p) {
    const u32 leaf = leaf_of(v, leaves[p]);
    const u32 y = find(c, leaf);
    if (x == y) continue;
    if (count + 1 >= nb) return false;
    const u32 a = top(c, x), b = top(c, y);
    const u32 ml = std::min(c[x].minleaf, c[y].minleaf);
    u32 s = x, l = y;
    if (c[x].size < c[y].size) s = y, l = x;  // union par TAILLE
    v.events[count] = Event{r, a, b, ml, s};
    v.event_cell[count] = t;
    c[l].up = s;
    c[s].size += c[l].size;
    c[s].minleaf = ml;
    c[s].last = count;
    v.attach_parent[l] = s;
    v.attach_rank[l] = r;
    ++count;
    x = s;
  }
  v.element[t] = x;
  top_out = top(c, x);
  return true;
}

}  // namespace

Outcome resolve_leaves(const ForestInput& in, const OrderForest& f, std::span<u32> leaves, u64 begin, u64 end,
                       ForestWork& counters) noexcept {
  const u64 nc = in.cell_ball.size();
  u64 births = 0, cells = 0;
  for (u64 t = begin; t < end; ++t) {
    const u32 r = idx(in.cell_rank[t]);
    for (u64 p = in.rep_offsets[t]; p < in.rep_offsets[t + 1]; ++p) {
      const u32 target = in.targets[p], index = target_index(target);
      if (target == kNoTarget) return fail(Reason::tower_invariant);
      if (target_is_cell(target)) {
        if (index >= nc || idx(in.cell_rank[index]) >= r) return fail(Reason::tower_invariant);
        leaves[p] = target;
        ++cells;
      } else {
        if (index >= f.births || idx(in.birth_rank[index]) >= r) return fail(Reason::tower_invariant);
        leaves[p] = f.birth_node[index];
        ++births;
      }
    }
  }
  counters.birth_targets += births;
  counters.cell_targets += cells;
  return {};
}

Outcome run_kernel(const ForestInput& input, std::span<const u32> leaves, OrderForest& forest, OrderWork& work,
                   ForestWork& counters, MemoryBudget& budget) noexcept {
  const u32 nb = forest.births;
  const u64 nc = input.cell_ball.size();
  if (nb == 0 || !operand_domain(nb) || !operand_domain(nc)) return fail(Reason::tower_capacity);
  if (leaves.size() != input.targets.size()) return fail(Reason::tower_invariant);
  Buffer<UnionCell> cells;
  Buffer<u32> element;
  MHGP12_TRY(cells.allocate(nb, budget));
  MHGP12_TRY(element.allocate(nc, budget));
  MHGP12_TRY(work.events.allocate(nb - 1, budget));
  MHGP12_TRY(forest.event_cell.allocate(nb - 1, budget));
  MHGP12_TRY(work.cell_top.allocate(nc, budget));
  MHGP12_TRY(forest.attach_parent.allocate(nb, budget));
  MHGP12_TRY(forest.attach_rank.allocate(nb, budget));
  for (u32 i = 0; i < nb; ++i) {
    cells[i] = UnionCell{i, 1, kNone, i};
    forest.attach_parent[i] = kNone;
    forest.attach_rank[i] = 0;
  }
  KernelView view{cells.data(), work.events.data(), forest.event_cell.data(), forest.attach_parent.data(),
                  forest.attach_rank.data(), element.data()};
  u32 count = 0;
  for (u64 t = 0; t < nc; ++t) {
    if (input.rep_offsets[t + 1] <= input.rep_offsets[t]) return fail(Reason::tower_invariant);
    u32 top_node = kNone;
    const u32 before = count;
    if (!process_cell(input, leaves.data(), nb, view, static_cast<u32>(t), count, top_node))
      return fail(Reason::tower_invariant);
    work.cell_top[t] = top_node;
    counters.retained_cells += count != before;
  }
  // Racine unique (contrat, paragraphe 5) : une union de moins que de naissances.
  if (u64{count} + 1 != nb) return fail(Reason::tower_invariant);
  work.event_count = count;
  counters.events = count;
  counters.attaches = count;
  return {};
}

Outcome build_history(OrderForest& forest, const OrderWork& work, ForestWork& counters,
                      MemoryBudget& budget) noexcept {
  const u32 nb = forest.births, ne = work.event_count;
  // Profondeur d'attache (LEM-T5 (i)) : en remontant les evenements, profondeur(perdant) = profondeur(survivant) + 1 ;
  // le survivant ne perd qu'a un evenement posterieur, deja vu. Saturee a 255 (compteur seulement).
  Buffer<u8> depth;
  MHGP12_TRY(depth.allocate(nb, budget));
  for (u32 i = 0; i < nb; ++i) depth[i] = 0;
  u64 deepest = 0;
  for (u32 e = ne; e-- > 0;) {
    const Event& ev = work.events[e];
    const u32 ra = (ev.a & kEventBit) ? work.events[ev.a & kEventIndex].surv : ev.a;
    const u32 rb = (ev.b & kEventBit) ? work.events[ev.b & kEventIndex].surv : ev.b;
    const u32 loser = ra == ev.surv ? rb : ra;
    if (loser >= nb || forest.attach_parent[loser] != ev.surv) return fail(Reason::tower_invariant);
    depth[loser] = static_cast<u8>(std::min(255, depth[ev.surv] + 1));
    deepest = std::max<u64>(deepest, depth[loser]);
  }
  counters.max_attach_depth = deepest;
  // Evenements par survivant (CSR), dans l'ordre de traitement, donc par rang croissant au sens large.
  MHGP12_TRY(forest.survivor_events.off.allocate(u64{nb} + 1, budget));
  if (ne > 0) MHGP12_TRY(forest.survivor_events.val.allocate(ne, budget));
  auto& off = forest.survivor_events.off;
  for (u64 i = 0; i <= nb; ++i) off[i] = 0;
  for (u32 e = 0; e < ne; ++e) ++off[u64{work.events[e].surv} + 1];
  for (u64 i = 0; i < nb; ++i) off[i + 1] += off[i];
  Buffer<u64> fill;
  MHGP12_TRY(fill.allocate(nb, budget));
  for (u64 i = 0; i < nb; ++i) fill[i] = off[i];
  for (u32 e = 0; e < ne; ++e) forest.survivor_events.val[fill[work.events[e].surv]++] = e;
  return {};
}

}  // namespace mhgp12::tower::detail
