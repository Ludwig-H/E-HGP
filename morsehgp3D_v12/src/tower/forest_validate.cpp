// Invariants globaux de la tour (contrat, paragraphe 9.5), lus sur le registre seul, hors du chemin chronometre : une
// racine par ordre, une arete par noeud sauf la racine, enfants tries de rang strictement inferieur, naissances
// d'abord par rang croissant, fusions par (rang, plus petite naissance) croissant, profondeur d'attache au plus
// floor(log2(naissances)) (LEM-T5 (i), union par taille) ; a k >= 2, verticales vivantes a la coupe fermee de leur
// noeud et naturelles (chaque enfant d'une fusion remonte, a la coupe fermee de la fusion, a l'image de la fusion :
// theoreme F (ii)), par activation monotone des fusions de l'ordre inferieur et chemins comprimes (lecteur strict de
// la v11, bench/full_semantic.py), jamais par une remontee complete par arete ; branches des hyperaretes retenues
// vivantes a la coupe ouverte de leur rang. Refus tower_invariant. Le reste de l'historique de LEM-T5 n'est pas relu
// (forest.hpp, validate_forests).
#include <algorithm>
#include <bit>

#include "tower/forest_internal.hpp"

namespace mhgp12::tower {
namespace {

// Forme de l'arbre d'un ordre.
Outcome check_shape(const OrderForest& f) noexcept {
  const u32 nn = f.nodes();
  if (f.births == 0 || nn < f.births || f.parent.size() != nn || f.minleaf.size() != nn ||
      f.children.off.size() != u64{nn} + 1 || !f.children.well_formed() || f.root >= nn ||
      f.children.val.size() + 1 != nn)
    return fail(Reason::tower_invariant);
  for (u32 v = 0; v < nn; ++v) {
    if ((f.parent[v] == kNone) != (v == f.root)) return fail(Reason::tower_invariant);
    const u64 b = f.children.off[v], e = f.children.off[u64{v} + 1];
    if (v < f.births ? e != b : e - b < 2) return fail(Reason::tower_invariant);
    for (u64 j = b; j < e; ++j) {
      const u32 c = f.children.val[j];
      if (c >= nn || f.parent[c] != v || f.rank[c] >= f.rank[v] || (j > b && c <= f.children.val[j - 1]))
        return fail(Reason::tower_invariant);
    }
    u32 smallest = v < f.births ? v : kNone;
    for (u64 j = b; j < e; ++j) smallest = std::min(smallest, f.minleaf[f.children.val[j]]);
    if (f.minleaf[v] != smallest) return fail(Reason::tower_invariant);
    if (v > 0 && v < f.births && f.rank[v] < f.rank[v - 1]) return fail(Reason::tower_invariant);
    if (v > f.births && (f.rank[v] < f.rank[v - 1] || (f.rank[v] == f.rank[v - 1] && f.minleaf[v] <= f.minleaf[v - 1])))
      return fail(Reason::tower_invariant);
  }
  return {};
}

// Profondeur d'attache : chemins remontes avec memoire (pile bornee : la borne est la question posee).
Outcome check_attach_depth(const OrderForest& f, MemoryBudget& budget) noexcept {
  const u32 nb = f.births;
  if (f.attach_parent.size() != nb) return fail(Reason::tower_invariant);
  const u64 bound = static_cast<u64>(std::bit_width(u64{nb}) - 1);  // floor(log2(nb))
  Buffer<u8> depth;
  MHGP12_TRY(depth.allocate(nb, budget));
  for (u32 i = 0; i < nb; ++i) depth[i] = 255;  // inconnue
  std::array<u32, 64> stack{};
  for (u32 leaf = 0; leaf < nb; ++leaf) {
    u32 x = leaf, n = 0;
    while (depth[x] == 255 && f.attach_parent[x] != kNone) {
      if (n == stack.size() || f.attach_parent[x] >= nb) return fail(Reason::tower_invariant);
      stack[n++] = x;
      x = f.attach_parent[x];
    }
    if (depth[x] == 255) depth[x] = 0;
    while (n > 0) {
      const u32 y = stack[--n];
      depth[y] = static_cast<u8>(depth[f.attach_parent[y]] + 1);
      if (depth[y] > bound) return fail(Reason::tower_invariant);
    }
  }
  return {};
}

// Registre : cellules retenues croissantes, rangs croissants au sens large ; chaque ligne de branches a au moins deux
// noeuds, croissants, vivants a la coupe OUVERTE du rang de la cellule (rang < r, parent absent ou de rang >= r).
Outcome check_branches(const OrderForest& f) noexcept {
  const u64 rows = f.retained_cell.size();
  if (f.retained_ball.size() != rows || f.retained_rank.size() != rows || f.branches.off.size() != rows + 1 ||
      !f.branches.well_formed())
    return fail(Reason::tower_invariant);
  for (u64 j = 0; j < rows; ++j) {
    if (j > 0 && (f.retained_cell[j] <= f.retained_cell[j - 1] || f.retained_rank[j] < f.retained_rank[j - 1]))
      return fail(Reason::tower_invariant);
    const u32 r = f.retained_rank[j];
    const u64 b = f.branches.off[j], e = f.branches.off[j + 1];
    if (e - b < 2) return fail(Reason::tower_invariant);
    for (u64 p = b; p < e; ++p) {
      const u32 v = f.branches.val[p];
      if (v >= f.nodes() || (p > b && v <= f.branches.val[p - 1]) || f.rank[v] >= r ||
          (f.parent[v] != kNone && f.rank[f.parent[v]] < r))
        return fail(Reason::tower_invariant);
    }
  }
  return {};
}

// Verticales d'un ordre k >= 2 contre l'ordre k - 1.
Outcome check_verticals(const OrderForest& up, const OrderForest& low, MemoryBudget& budget) noexcept {
  const u32 nn = up.nodes(), nl = low.nodes();
  if (up.lower.size() != nn) return fail(Reason::tower_invariant);
  for (u32 v = 0; v < nn; ++v) {  // vivante a la coupe fermee du niveau de v
    const u32 u = up.lower[v];
    if (u >= nl || low.rank[u] > up.rank[v]) return fail(Reason::tower_invariant);
    if (low.parent[u] != kNone && low.rank[low.parent[u]] <= up.rank[v]) return fail(Reason::tower_invariant);
  }
  // Naturalite : fusions hautes par rang croissant ; fusions basses activees quand leur rang ne depasse pas.
  Buffer<u32> active;
  MHGP12_TRY(active.allocate(nl, budget));
  for (u32 v = 0; v < nl; ++v) active[v] = v;
  auto find = [&active](u32 x) {
    u32 root = x;
    while (active[root] != root) root = active[root];
    while (active[x] != root) {
      const u32 next = active[x];
      active[x] = root;
      x = next;
    }
    return root;
  };
  u32 cursor = low.births;
  for (u32 v = up.births; v < nn; ++v) {
    while (cursor < nl && low.rank[cursor] <= up.rank[v]) {
      for (u64 j = low.children.off[cursor]; j < low.children.off[u64{cursor} + 1]; ++j)
        active[low.children.val[j]] = cursor;
      ++cursor;
    }
    for (u64 j = up.children.off[v]; j < up.children.off[u64{v} + 1]; ++j)
      if (find(up.lower[up.children.val[j]]) != up.lower[v]) return fail(Reason::tower_invariant);
  }
  return {};
}

}  // namespace

Outcome validate_forests(const TowerForests& forests, MemoryBudget& budget) noexcept {
  return guarded([&]() -> Outcome {
    if (forests.kmax < 1 || forests.kmax > kMaxOrder) return fail(Reason::tower_invariant);
    // Admission avant allocation (regle du socle, core/buffer.hpp) : un seul tampon vivant a la fois, les profondeurs
    // d'un ordre (un octet par naissance) ou les activations de l'ordre inferieur (quatre octets par noeud).
    u64 bytes = 0;
    for (u32 i = 0; i < forests.kmax; ++i) {
      bytes = std::max<u64>(bytes, forests.orders[i].births);
      if (i > 0) bytes = std::max<u64>(bytes, 4 * u64{forests.orders[i - 1].nodes()});
    }
    MHGP12_TRY(budget.admit(bytes));
    for (u32 i = 0; i < forests.kmax; ++i) {
      const OrderForest& f = forests.orders[i];
      if (f.k != i + 1) return fail(Reason::tower_invariant);
      MHGP12_TRY(check_shape(f));
      MHGP12_TRY(check_attach_depth(f, budget));
      MHGP12_TRY(check_branches(f));
      if (i > 0) MHGP12_TRY(check_verticals(f, forests.orders[i - 1], budget));
      else if (!f.lower.empty()) return fail(Reason::tower_invariant);
    }
    return {};
  });
}

}  // namespace mhgp12::tower
