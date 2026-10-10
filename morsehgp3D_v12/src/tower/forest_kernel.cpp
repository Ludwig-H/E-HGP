// Etage T : noyau union-find par TAILLE sans lots d'un ordre (D-F1, LEM-T4 ; CONCEPTION_TOUR.md de la v11, paragraphe
// 4.1), port du noyau de MES-M4, en deux temps (le noyau est REPRENABLE par cellules croissantes : la Session recouverte
// le fait avancer tranche de G par tranche de G, D-F2 ; build_forests le joue d'un trait) :
//   pre-passe (parallele par morceaux de cellules, hors du passage sequentiel) : la cible de chaque representant devient
//   sa feuille canonique (naissance, par son noeud) ou garde son codage << cellule >> ; la date de LEM-T3 est controlee
//   (rang de la cible strictement inferieur a celui de la cellule, donc cellule cible deja traitee) ;
//   noyau (sequentiel, un proprietaire par ordre) : cellules par rang croissant ; feuille de chaque representant (une
//   cible << cellule >> se lit comme l'element de la cellule deja traitee), relue a sa racine courante ; chaque union de
//   deux racines distinctes est un evenement binaire (rang, sommets des deux composantes, plus petite feuille,
//   survivant), avec sa cellule ; attache du perdant avec son rang ; sommet et element de la cellule apres traitement.
// Aucune allocation dans la boucle. Les plateaux sont rendus atomiques par la contraction (LEM-T4) : l'ordre des unions
// a rang egal et la regle d'union n'y changent rien ; l'union par taille borne l'historique d'attache (LEM-T5 (i)).
//
// Indices de racine (Session recouverte, levier I de T2-d-A6) : des taches d'aide remplacent les feuilles des
// tranches a venir par la racine de leur composante dans l'union-find VIVANT (hint_leaves). Une composante ne fait que
// grossir : la racine lue est, quand le noyau traite la cellule, dans la meme composante que la feuille d'origine ; le
// noyau ne lit d'une feuille que sa racine courante (find), donc sa sortie ne change pas, quel que soit
// l'entrelacement, avec le pont de publication decrit dans load_leaf : up est atomique relache ; les feuilles
// sont publiees en release et lues en acquire (les autres champs ne sont lus que par le noyau). Ces acces partages ne
// servent qu'a un ordre dont la chaine est engagee (A6c, kShared) ; sinon, et dans build_forests, le noyau est celui de
// la base, a acces ordinaires (aucun lecteur concurrent de l'union-find ni des feuilles).
#include <algorithm>
#include <atomic>

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

inline u32 load_up(UnionCell* c, u32 x) noexcept {
  return std::atomic_ref<u32>(c[x].up).load(std::memory_order_relaxed);
}

template <bool kShared>
inline u32 find(UnionCell* c, u32 x) noexcept {
  if constexpr (kShared) {
    for (;;) {
      const u32 up = load_up(c, x);
      if (up == x) return x;
      const u32 above = load_up(c, up);
      std::atomic_ref<u32>(c[x].up).store(above, std::memory_order_relaxed);  // demi-compression
      x = above;
    }
  } else {
    while (c[x].up != x) {
      c[x].up = c[c[x].up].up;  // demi-compression
      x = c[x].up;
    }
    return x;
  }
}

// Racine courante sans ecriture (taches d'indices) : chaque pointeur lu relie deux noeuds de la meme composante.
inline u32 find_read(UnionCell* c, u32 x) noexcept {
  for (u32 up = load_up(c, x); up != x; up = load_up(c, x)) x = up;
  return x;
}

// Feuille lue par le noyau (acquire) : si c'est un indice publie par une tache d'aide (store release dans
// hint_leaves), chaque pointeur up lu par l'aide precede sa publication, qui se synchronise avec cette lecture ; une
// ecriture ulterieure de up par le noyau lui est donc posterieure (happens-before) et l'aide ne peut pas l'avoir lue :
// l'indice est une racine du prefixe deja consomme (pont de publication de l'auditeur, CST-0242).
inline u32 load_leaf(u32* leaves, u64 p) noexcept {
  return std::atomic_ref<u32>(leaves[p]).load(std::memory_order_acquire);
}

// Feuille lue par le noyau : partagee (acquire, ci-dessus) si la chaine de l'ordre est engagee, ordinaire sinon.
template <bool kShared>
inline u32 kernel_leaf(u32* leaves, u64 p) noexcept {
  if constexpr (kShared) return load_leaf(leaves, p);
  else return leaves[p];
}

inline u32 top(const UnionCell* c, u32 x) noexcept { return c[x].last == kNone ? x : (c[x].last | kEventBit); }

// Feuille d'un representant : sa naissance canonique, ou l'element de la cellule cible (deja traitee).
inline u32 leaf_of(const KernelView& v, u32 code) noexcept {
  return target_is_cell(code) ? v.element[target_index(code)] : code;
}

// Une cellule : unions de ses representants ; faux si une union deborderait (au plus naissances - 1 : jamais dans
// le domaine). Prechargement des feuilles suivantes borne a `ready` (representants dont les feuilles sont ecrites :
// la Session ne lit jamais une tranche de G non terminee).
template <bool kShared>
bool process_cell(const ForestInput& in, u32* leaves, u64 ready, u32 nb, KernelView& v, u32 t, u32& count,
                  u32& top_out) noexcept {
  const u64 b0 = in.rep_offsets[t], e0 = in.rep_offsets[t + 1], nr = ready;
  const u32 r = idx(in.cell_rank[t]);
  UnionCell* c = v.cells;
  for (u64 p = b0 + 16; p < e0 + 16 && p < nr; ++p) {
    const u32 ahead = kernel_leaf<kShared>(leaves, p);
    if (!target_is_cell(ahead)) __builtin_prefetch(&c[ahead]);
  }
  u32 x = find<kShared>(c, leaf_of(v, kernel_leaf<kShared>(leaves, b0)));
  for (u64 p = b0 + 1; p < e0; ++p) {
    const u32 leaf = leaf_of(v, kernel_leaf<kShared>(leaves, p));
    const u32 y = find<kShared>(c, leaf);
    if (x == y) continue;
    if (count + 1 >= nb) return false;
    const u32 a = top(c, x), b = top(c, y);
    const u32 ml = std::min(c[x].minleaf, c[y].minleaf);
    u32 s = x, l = y;
    if (c[x].size < c[y].size) s = y, l = x;  // union par TAILLE
    v.events[count] = Event{r, a, b, ml, s};
    v.event_cell[count] = t;
    if constexpr (kShared) std::atomic_ref<u32>(c[l].up).store(s, std::memory_order_relaxed);
    else c[l].up = s;
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

Outcome open_kernel(const ForestInput& input, OrderForest& forest, OrderWork& work, MemoryBudget& budget) noexcept {
  const u32 nb = forest.births;
  const u64 nc = input.cell_ball.size();
  if (nb == 0 || !operand_domain(nb) || !operand_domain(nc)) return fail(Reason::tower_capacity);
  MHGP12_TRY(work.cells.allocate(nb, budget));
  MHGP12_TRY(work.element.allocate(nc, budget));
  MHGP12_TRY(work.events.allocate(nb - 1, budget));
  MHGP12_TRY(forest.event_cell.allocate(nb - 1, budget));
  MHGP12_TRY(work.cell_top.allocate(nc, budget));
  MHGP12_TRY(forest.attach_parent.allocate(nb, budget));
  MHGP12_TRY(forest.attach_rank.allocate(nb, budget));
  for (u32 i = 0; i < nb; ++i) {
    work.cells[i] = UnionCell{i, 1, kNone, i};
    forest.attach_parent[i] = kNone;
    forest.attach_rank[i] = 0;
  }
  work.next_cell = 0;
  work.event_count = 0;
  return {};
}

namespace {

// Cellules [next_cell, end_cell) du noyau : acces partages (kShared, chaine engagee, taches d'indices possibles) ou
// ordinaires (code de la base).
template <bool kShared>
Outcome advance_cells(const ForestInput& input, std::span<u32> leaves, OrderForest& forest, OrderWork& work,
                      ForestWork& counters, u64 end_cell) noexcept {
  const u32 nb = forest.births;
  const u64 nc = input.cell_ball.size();
  if (leaves.size() != input.targets.size() || end_cell > nc || work.cells.size() != nb)
    return fail(Reason::tower_invariant);
  KernelView view{work.cells.data(), work.events.data(), forest.event_cell.data(), forest.attach_parent.data(),
                  forest.attach_rank.data(), work.element.data()};
  u32 count = work.event_count;
  const u64 ready = input.rep_offsets[end_cell];
  for (u64 t = work.next_cell; t < end_cell; ++t) {
    if (input.rep_offsets[t + 1] <= input.rep_offsets[t]) return fail(Reason::tower_invariant);
    u32 top_node = kNone;
    const u32 before = count;
    if (!process_cell<kShared>(input, leaves.data(), ready, nb, view, static_cast<u32>(t), count, top_node))
      return fail(Reason::tower_invariant);
    work.cell_top[t] = top_node;
    counters.retained_cells += count != before;
  }
  work.event_count = count;
  work.next_cell = std::max(work.next_cell, end_cell);
  return {};
}

}  // namespace

Outcome advance_kernel(const ForestInput& input, std::span<u32> leaves, OrderForest& forest, OrderWork& work,
                       ForestWork& counters, u64 end_cell, bool shared) noexcept {
  return shared ? advance_cells<true>(input, leaves, forest, work, counters, end_cell)
                : advance_cells<false>(input, leaves, forest, work, counters, end_cell);
}

Outcome close_kernel(const ForestInput& input, OrderForest& forest, OrderWork& work, ForestWork& counters) noexcept {
  if (work.next_cell != input.cell_ball.size()) return fail(Reason::tower_invariant);
  // Racine unique (contrat, paragraphe 5) : une union de moins que de naissances.
  if (u64{work.event_count} + 1 != forest.births) return fail(Reason::tower_invariant);
  counters.events = work.event_count;
  counters.attaches = work.event_count;
  work.cells.reset();
  work.element.reset();
  return {};
}

Outcome run_kernel(const ForestInput& input, std::span<u32> leaves, OrderForest& forest, OrderWork& work,
                   ForestWork& counters, MemoryBudget& budget) noexcept {
  if (leaves.size() != input.targets.size()) return fail(Reason::tower_invariant);
  MHGP12_TRY(open_kernel(input, forest, work, budget));
  MHGP12_TRY(advance_kernel(input, leaves, forest, work, counters, input.cell_ball.size()));
  return close_kernel(input, forest, work, counters);
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

u64 hint_leaves(const ForestInput& input, OrderWork& work, u64 begin, u64 end, u64 processed) noexcept {
  UnionCell* c = work.cells.data();
  u32* leaves = work.leaves.data();
  u64 hinted = 0;
  for (u64 p = input.rep_offsets[begin]; p < input.rep_offsets[end]; ++p) {
    const u32 code = load_leaf(leaves, p);
    u32 node = code;
    if (target_is_cell(code)) {
      if (target_index(code) >= processed) continue;  // cellule cible pas encore traitee : feuille gardee
      node = work.element[target_index(code)];
    }
    // release : publie l'indice apres toutes ses lectures de up (voir load_leaf).
    std::atomic_ref<u32>(leaves[p]).store(find_read(c, node), std::memory_order_release);
    ++hinted;
  }
  return hinted;
}

Outcome check_history(const OrderForest& forest, const OrderWork& work, u64 begin, u64 end) noexcept {
  const u32 nb = forest.births;
  for (u64 e = begin; e < end; ++e) {
    const Event& ev = work.events[e];
    const u32 ra = (ev.a & kEventBit) ? work.events[ev.a & kEventIndex].surv : ev.a;
    const u32 rb = (ev.b & kEventBit) ? work.events[ev.b & kEventIndex].surv : ev.b;
    const u32 loser = ra == ev.surv ? rb : ra;
    if (loser >= nb || forest.attach_parent[loser] != ev.surv) return fail(Reason::tower_invariant);
  }
  return {};
}

void history_depth(const OrderForest& forest, u64 begin, u64 end, ForestWork& counters) noexcept {
  // profondeur d'un noeud = longueur de sa chaine d'attaches (meme valeur que la passe arriere de build_history,
  // saturee a 255)
  u64 deepest = counters.max_attach_depth;
  for (u64 v = begin; v < end; ++v) {
    u64 d = 0;
    for (u32 x = static_cast<u32>(v); forest.attach_parent[x] != kNone && d < 255; x = forest.attach_parent[x]) ++d;
    deepest = std::max(deepest, d);
  }
  counters.max_attach_depth = deepest;
}

Outcome build_survivor_events(OrderForest& forest, const OrderWork& work, MemoryBudget& budget) noexcept {
  const u32 nb = forest.births, ne = work.event_count;
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
