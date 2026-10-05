// Selection et etiquettes de la tete plate (tranche S10) : port explicite de select et labels
// (bench/points_flat.py:765-878) et de l'ordre d'entree (specification, paragraphe 6.5).
//
// EOM N-aire, clusters dans l'ordre de naissance (enfants avant parents) : une feuille est retenue ; un cluster a
// enfants compare S(C) a la somme des S^ de ses enfants, par encadrements entiers, puis, s'ils ne separent pas, par le
// signe exact de la somme des poids entiers par plateau (score propre moins scores retenus du sous-arbre). Le parent
// l'emporte si S(C) > somme ou sur egalite CERTIFIEE ; S^(C) vaut alors S(C), sinon la somme. Feuilles : les clusters
// sans enfant. Racine exclue. Passe descendante : un cluster retenu masque ses descendants. Etiquette d'un site : plus
// petit PointId du cluster retenu qui le contient, -1 pour le bruit.
// Ce qui change : encadrements entiers au lieu du filtre flottant ; les poids d'un repli exact sont fusionnes par
// plateau avant tout radical (memes decisions, moins de termes) ; refus de l'appel entier au premier
// radical_sign_budget (reponse de l'auditeur, 6eba951df).
#include <algorithm>

#include "head/internal.hpp"
#include "points/points.hpp"

namespace mhgp11::head {

Outcome CatalogueLevels::root(u32 rank, u128& out) const noexcept {
  MHGP11_CHECK(rank < levels_.size(), head_invariant);
  return num::RootTable::root_of(levels_[rank], out);
}

Outcome CatalogueLevels::value(u32 rank, num::Rational& out) const noexcept {
  MHGP11_CHECK(rank < levels_.size(), head_invariant);
  return num::Rational::from_level(levels_[rank], out);
}

namespace {

using detail::Fixed;

// Intervalle d'un score : [lo, hi] dans l'unite 2^192, ou ouvert (un plateau sans encadrement y compte).
struct Interval {
  Fixed lo, hi;
  bool open = false;
};

Outcome accumulate(Fixed& acc, const Fixed& value, u64 factor, bool negate) noexcept {
  Fixed scaled;
  const auto product = num::multiply(value, num::Wide<1>::from_u64(factor));
  MHGP11_CHECK(num::resize<6>(product, scaled), arithmetic_invariant);
  if (negate) scaled = scaled.negated();
  MHGP11_CHECK(num::add(acc, scaled, acc), arithmetic_invariant);
  return {};
}

struct Selector {
  const TreeView& tree;
  const LevelSource& levels;
  const detail::Condensed& cond;
  const detail::Brackets& brackets;
  FlatParams params;
  MemoryBudget& budget;
  FlatStats& stats;
  Buffer<Interval> score, hat;
  Buffer<u8> chosen;
  Buffer<detail::Weight> weights;
  Buffer<u32> stack;

  bool root(u32 c) const noexcept { return cond.parent[c] == kNone; }

  Outcome own_score(u32 c, Interval& out) noexcept {
    out = Interval{};
    for (u64 j = cond.join_off[c]; j < cond.join_off[c + 1]; ++j) {
      const u32 p = cond.join_plateau[j];
      MHGP11_CHECK(brackets.zero[p] == 0, head_invariant);  // phi(0) demande
      if (brackets.open[p]) {
        out.open = true;
        continue;
      }
      MHGP11_TRY(accumulate(out.lo, brackets.lo[p], cond.join_count[j], false));
      MHGP11_TRY(accumulate(out.hi, brackets.hi[p], cond.join_count[j], false));
    }
    const u32 top = cond.top[c];
    if (top != kNone && cond.size[c] != 0) {
      MHGP11_CHECK(brackets.zero[top] == 0, head_invariant);
      if (brackets.open[top]) {
        out.open = true;
      } else {
        MHGP11_TRY(accumulate(out.lo, brackets.hi[top], cond.size[c], true));
        MHGP11_TRY(accumulate(out.hi, brackets.lo[top], cond.size[c], true));
      }
    }
    return {};
  }

  // Poids entiers du score propre de c, de signe sign.
  Outcome push_score(u32 c, i64 sign, u64& used) noexcept {
    for (u64 j = cond.join_off[c]; j < cond.join_off[c + 1]; ++j) {
      MHGP11_CHECK(used < weights.size() && cond.join_count[j] <= u64{1} << 62, head_invariant);
      weights[used++] = {cond.join_plateau[j], sign * static_cast<i64>(cond.join_count[j])};
    }
    if (cond.top[c] != kNone && cond.size[c] != 0) {
      MHGP11_CHECK(used < weights.size() && cond.size[c] <= u64{1} << 62, head_invariant);
      weights[used++] = {cond.top[c], -sign * static_cast<i64>(cond.size[c])};
    }
    return {};
  }

  // Signe exact de S(c) - somme des S^ de ses enfants (sous-arbres deja decides).
  Outcome exact_decision(u32 c, int& sign) noexcept {
    u64 used = 0, depth = 0;
    MHGP11_TRY(push_score(c, 1, used));
    for (const u32 d : cond.children.row(c)) stack[depth++] = d;
    while (depth != 0) {
      const u32 x = stack[--depth];
      if (chosen[x] || cond.children.row(x).empty()) {
        MHGP11_TRY(push_score(x, -1, used));
        continue;
      }
      for (const u32 d : cond.children.row(x)) {
        MHGP11_CHECK(depth < stack.size(), head_invariant);
        stack[depth++] = d;
      }
    }
    std::sort(weights.begin(), weights.begin() + used,
              [](const detail::Weight& a, const detail::Weight& b) { return a.plateau < b.plateau; });
    u64 kept = 0;
    for (u64 i = 0; i < used;) {
      const u32 p = weights[i].plateau;
      i64 total = 0;
      for (; i < used && weights[i].plateau == p; ++i) {
        MHGP11_CHECK(!__builtin_add_overflow(total, weights[i].weight, &total), arithmetic_invariant);
      }
      if (total != 0) weights[kept++] = {p, total};
    }
    return detail::exact_sign(tree, levels, params.z, std::span<const detail::Weight>(weights.data(), kept), budget,
                              sign);
  }

  Outcome run() noexcept {
    const u32 count = cond.count;
    MHGP11_TRY(budget.admit(u64{count} * (2 * sizeof(Interval) + 1 + 4) +
                            sizeof(detail::Weight) * (cond.join_plateau.size() + count)));
    MHGP11_TRY(score.allocate(count, budget));
    MHGP11_TRY(hat.allocate(count, budget));
    MHGP11_TRY(chosen.allocate(count, budget));
    std::fill(chosen.begin(), chosen.end(), u8{0});
    if (params.selection == Selection::leaves) {
      for (u32 c = 0; c < count; ++c) chosen[c] = !root(c) && cond.children.row(c).empty();
      return {};
    }
    MHGP11_TRY(stack.allocate(count, budget));
    MHGP11_TRY(weights.allocate(cond.join_plateau.size() + count, budget));
    for (u32 c = 0; c < count; ++c) {
      if (root(c)) continue;
      MHGP11_TRY(own_score(c, score[c]));
      const auto kids = cond.children.row(c);
      if (kids.empty()) {
        chosen[c] = 1;
        hat[c] = score[c];
        continue;
      }
      Interval sub;
      for (const u32 d : kids) {
        sub.open = sub.open || hat[d].open;
        MHGP11_CHECK(num::add(sub.lo, hat[d].lo, sub.lo) && num::add(sub.hi, hat[d].hi, sub.hi),
                     arithmetic_invariant);
      }
      ++stats.decisions;
      int sign = 0;
      bool decided = false;
      if (!score[c].open && !sub.open) {
        Fixed low, high;  // S - somme dans [S_lo - T_hi, S_hi - T_lo]
        MHGP11_CHECK(num::subtract(score[c].lo, sub.hi, low) && num::subtract(score[c].hi, sub.lo, high),
                     arithmetic_invariant);
        if (low.sign() > 0) sign = 1, decided = true;
        else if (high.sign() < 0) sign = -1, decided = true;
      }
      if (!decided) {
        ++stats.exact;
        MHGP11_TRY(exact_decision(c, sign));
        if (sign == 0) ++stats.equalities;
      }
      if (sign >= 0) {  // parent sur egalite certifiee
        chosen[c] = 1;
        hat[c] = score[c];
      } else {
        hat[c] = sub;
      }
    }
    return {};
  }
};

}  // namespace

Result<SiteLabels> flat_sites(const TreeView& tree, const LevelSource& levels, FlatParams params,
                              MemoryBudget& budget) noexcept {
  if (params.mcs < 2 || params.z < 1 || params.z > 3) return fail(Reason::parameter_out_of_range);
  if (params.selection != Selection::eom && params.selection != Selection::leaves)
    return fail(Reason::parameter_out_of_range);
  SiteLabels out;
  detail::Condensed cond;
  MHGP11_TRY(detail::condense(tree, params.mcs, budget, cond));
  out.stats.clusters = cond.count;
  detail::Brackets brackets;
  if (params.selection == Selection::eom) {
    MHGP11_TRY(detail::bracket_plateaus(tree, levels, params.z, budget, brackets));
    out.stats.unbracketed = brackets.unbracketed;
  }
  Selector selector{tree, levels, cond, brackets, params, budget, out.stats, {}, {}, {}, {}, {}};
  MHGP11_TRY(selector.run());
  // Passe descendante (parents apres enfants : on descend), puis proprietaire retenu de chaque cluster.
  const u32 count = cond.count;
  const u64 n = tree.site_block.size();
  Buffer<u32> owner;
  Buffer<u8> selected, blocked;
  Buffer<PointId> canon;
  MHGP11_TRY(budget.admit(u64{count} * (4 + 1 + 1 + sizeof(PointId)) + 8 * n));
  MHGP11_TRY(owner.allocate(count, budget));
  MHGP11_TRY(selected.allocate(count, budget));
  MHGP11_TRY(blocked.allocate(count, budget));
  MHGP11_TRY(canon.allocate(count, budget));
  for (u32 i = count; i-- > 0;) {
    const u32 up = cond.parent[i];
    blocked[i] = selected[i] = 0;
    if (up != kNone && (blocked[up] || selected[up])) {
      blocked[i] = 1;
      continue;
    }
    if (selector.chosen[i] && up != kNone) selected[i] = 1;
  }
  for (u32 i = count; i-- > 0;) {
    const u32 up = cond.parent[i];
    owner[i] = up != kNone && owner[up] != kNone ? owner[up] : (selected[i] ? i : kNone);
    out.stats.selected += selected[i];
  }
  std::fill(canon.begin(), canon.end(), make_id<PointId>(kNone));
  for (u64 s = 0; s < n; ++s) {
    const u32 c = cond.first[s];
    if (c == kNone || owner[c] == kNone) continue;
    const u32 o = owner[c];
    if (idx(canon[o]) == kNone || idx(tree.site_ids[s]) < idx(canon[o])) canon[o] = tree.site_ids[s];
  }
  MHGP11_TRY(out.labels.allocate(n, budget));
  for (u64 s = 0; s < n; ++s) {
    const u32 c = cond.first[s];
    const u32 o = c == kNone ? kNone : owner[c];
    out.labels[s] = o == kNone ? i64{-1} : static_cast<i64>(idx(canon[o]));
    out.stats.noise += o == kNone;
  }
  return out;
}

Result<Buffer<i64>> in_input_order(std::span<const i64> site_labels, std::span<const PointId> site_ids,
                                   std::span<const PointId> input_ids, MemoryBudget& budget) noexcept {
  const u64 n = site_ids.size();
  if (site_labels.size() != n || input_ids.size() != n) return fail(Reason::head_invariant);
  MHGP11_TRY(budget.admit(8 * n + 8 * n + n));
  Buffer<u64> keyed;
  MHGP11_TRY(keyed.allocate(n, budget));
  for (u64 s = 0; s < n; ++s) keyed[s] = (u64{idx(site_ids[s])} << 32) | s;
  std::sort(keyed.begin(), keyed.end());
  Buffer<i64> out;
  Buffer<u8> seen;
  MHGP11_TRY(out.allocate(n, budget));
  MHGP11_TRY(seen.allocate(n, budget));
  std::fill(seen.begin(), seen.end(), u8{0});
  for (u64 i = 0; i < n; ++i) {
    const u64 key = u64{idx(input_ids[i])} << 32;
    const u64* hit = std::lower_bound(keyed.begin(), keyed.end(), key);
    if (hit == keyed.end() || (*hit >> 32) != (key >> 32)) return fail(Reason::head_invariant);
    const u64 s = *hit & 0xFFFFFFFFull;
    if (seen[s]) return fail(Reason::head_invariant);
    seen[s] = 1;
    out[i] = site_labels[s];
  }
  return out;
}

Result<FlatLabels> flat(const points::PointHierarchy& hierarchy, std::span<const num::Level> levels,
                        std::span<const PointId> site_ids, std::span<const PointId> input_ids, FlatParams params,
                        MemoryBudget& budget) noexcept {
  const points::PointTree& t = hierarchy.tree();
  const TreeView view{t.plateau_t(),    t.plateau_m(),   t.plateau_q(),  t.block_plateau(),
                      t.block_parent(), t.site_block(), t.site_plateau(), site_ids};
  const CatalogueLevels source(levels);
  auto sites = flat_sites(view, source, params, budget);
  if (!sites.ok()) return sites.outcome();
  SiteLabels made = std::move(sites).take();
  auto ordered = in_input_order(made.labels.span(), site_ids, input_ids, budget);
  if (!ordered.ok()) return ordered.outcome();
  FlatLabels out;
  out.labels = std::move(ordered).take();
  out.stats = made.stats;
  return out;
}

}  // namespace mhgp11::head
