// MorseHGP3D v6 — profondeur et census exacts d'une boule par sa forme
// primitive P(z) = A|z|² + B·z + C.
//
// Descente separable par axe : chaque axe contribue A t² + B_i t, parabole
// convexe dont le minimum de reseau sur [lo,hi] est a l'un des entiers voisins
// du sommet −B_i/(2A) (clippes) et le maximum aux bornes.
//
// PASSE 1 — `ball_depth_at_least(h)` : decide |I_B| >= h AVANT toute
// materialisation. Decisions (les inegalites comptent) :
//   mn >= 0 : rien de STRICTEMENT interieur (coquille incluse) — elague ;
//   mx <  0 : tout le nœud est strictement interieur — range-add O(1)
//             (STRICT : a mx == 0 une coquille serait comptee — mutant
//             `range-add-max-le-zero`) ;
//   sinon scission ; a la feuille, test exact.
// PASSE 2 — `ball_census` : I_B (interieurs stricts) et U_B (coquille
// COMPLETE, supports inclus) ; plafonds explicites (interieurs : la boule ne
// sert aucun K <= K_max ; coquille : resource_exhausted), jamais une
// troncature. La passe 2 ne remplace jamais la passe 1 et reciproquement
// (mutant `skip-full-census` : la passe count-only ne connait pas U_B).
//
// PRECONDITION (V1, tranchee par l'auditeur) : positions DISTINCTES — le
// pipeline refuse les doublons avant tout appel. Les comptes utilisent
// neanmoins `range_weight` (multiplicites) pour rester coherents avec l'index ;
// un index a doublons n'est pas une entree contractuelle de ces fonctions.
// Un index a UNE position unique n'a aucun nœud interne : sa racine est la
// feuille `leaf_ref(0)` et se traite comme toute feuille (jamais comme le vide).
//
// PILE DE DESCENTE HISSEE (palier P4 d'echelle, docs/ECHELLE.md § 6.4) : les
// deux passes descendent l'arbre avec une pile explicite. Declaree DANS le
// corps, cette pile etait un `std::vector` construit puis detruit A CHAQUE
// BOULE — donc une allocation initiale plus les reallocations geometriques de
// sa croissance, multipliees par le nombre de candidats (passe 1) et de
// survivantes (passe 2) : des dizaines de millions d'allocations courtes par
// run, et la retention d'allocateur qui va avec. Les deux fonctions prennent
// desormais une pile FOURNIE PAR L'APPELANT (`scratch`), hissee au niveau de
// l'ouvrier exactement comme `census_balls` hisse deja ses tampons `in`/`sh`
// (expand.hpp) : videe a chaque boule, sa capacite survit d'une boule a
// l'autre, donc plus une seule allocation apres les premieres descentes.
//   - `scratch == nullptr` reste accepte (oracles, portes de device) : la
//     fonction possede alors sa pile, et le COMPTE de ces piles possedees est
//     publie dans `DepthStats::owned_stacks` — un compteur DETERMINISTE,
//     fonction de l'entree et du decoupage, jamais de l'allocateur ;
//   - mutant `census-stack-per-ball` : la pile fournie est IGNOREE et la
//     fonction se rabat sur la sienne, exactement le defaut supprime. L'objet
//     est INCHANGE (aucun digest, aucune cardinalite ne le voit) : seul un
//     PLAFOND sur `owned_stacks` le tue (porte mhgp7_perm_sort_gate).
#pragma once

#include <algorithm>
#include <vector>

#include "../core/intmath.hpp"
#include "../core/mutants.hpp"
#include "../lanes/keys.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp7 {

namespace census_detail {

struct AxisBounds {
  const BallKey& k;
  i128 axis_val(int i, i64 t) const { return k.a * ((i128)t * t) + k.b[i] * t; }
  i128 axis_min(int i, i64 lo, i64 hi) const {
    const i64 t1 = (i64)floor_div128(-k.b[i], 2 * k.a);
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) { best = v; first = false; }
    }
    return best;
  }
  i128 axis_max(int i, i64 lo, i64 hi) const { return std::max(axis_val(i, lo), axis_val(i, hi)); }
  void bounds(const AxisBox& bz, i128* mn, i128* mx) const {
    *mn = k.c;
    *mx = k.c;
    for (int i = 0; i < 3; ++i) {
      *mn += axis_min(i, bz.lo[i], bz.hi[i]);
      *mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
  }
};

}  // namespace census_detail

struct DepthStats {
  u64 nodes = 0, leaf_tests = 0, range_add_mass = 0;
  // Nombre de descentes qui ont du POSSEDER leur pile (aucune pile fournie, ou
  // mutant `census-stack-per-ball`). Compteur deterministe : sur la voie
  // produit il vaut 0.
  u64 owned_stacks = 0;
};

namespace census_detail {
// Choisit la pile : celle de l'appelant si elle existe et si le mutant n'est
// pas arme, sinon la pile possedee (comptee). Dans les deux cas elle est videe
// et amorcee a la racine — le resultat ne depend d'aucun etat anterieur.
inline std::vector<NodeRef>& pick_stack(std::vector<NodeRef>* scratch, std::vector<NodeRef>* owned, NodeRef root,
                                        DepthStats* st) {
  const bool per_ball = MHGP7_MUTANT("census-stack-per-ball");
  std::vector<NodeRef>& s = (scratch != nullptr && !per_ball) ? *scratch : *owned;
  if (&s == owned && st) ++st->owned_stacks;
  s.clear();
  s.push_back(root);
  return s;
}
}  // namespace census_detail

// Rend true des que le compte des interieurs stricts atteint h ; sinon false
// et *count = compte EXACT (recoupe par la passe 2).
inline bool ball_depth_at_least(const CloudIndex& ix, const BallKey& k, u64 h, u64* count, DepthStats* st = nullptr,
                                std::vector<NodeRef>* scratch = nullptr) {
  *count = 0;
  if (ix.unique_count() == 0) return h == 0;
  const bool range_le = MHGP7_MUTANT("range-add-max-le-zero");
  const census_detail::AxisBounds ab{k};
  u64 c = 0;
  std::vector<NodeRef> owned;
  std::vector<NodeRef>& stack = census_detail::pick_stack(scratch, &owned, ix.root(), st);
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    if (st) ++st->nodes;
    i128 mn, mx;
    ab.bounds(ix.box_of(z), &mn, &mx);
    if (mn >= 0) continue;
    if (mx < 0 || (range_le && mx <= 0)) {
      const NodeRange r = ix.range_of(z);
      const u64 w = ix.range_weight(r.first, r.last);
      if (st) st->range_add_mass += w;
      c += w;
      if (c >= h) return true;
      continue;
    }
    if (is_leaf(z)) {
      if (st) ++st->leaf_tests;
      const i32 u = leaf_index(z);
      if (k.power(ix.upos[(size_t)u]) < 0) {
        c += ix.range_weight(u, u);
        if (c >= h) return true;
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  *count = c;
  return false;
}

enum class CensusStatus { kOk, kInteriorOverflow, kShellOverflow };

inline CensusStatus ball_census(const CloudIndex& ix, const BallKey& k, size_t interior_cap, size_t shell_cap,
                                std::vector<i32>* interior, std::vector<i32>* shell, DepthStats* st = nullptr,
                                std::vector<NodeRef>* scratch = nullptr) {
  interior->clear();
  shell->clear();
  if (ix.unique_count() == 0) return CensusStatus::kOk;
  const bool nonstrict = MHGP7_MUTANT("census-nonstrict");
  const census_detail::AxisBounds ab{k};
  std::vector<NodeRef> owned;
  std::vector<NodeRef>& stack = census_detail::pick_stack(scratch, &owned, ix.root(), st);
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    if (st) ++st->nodes;
    i128 mn, mx;
    ab.bounds(ix.box_of(z), &mn, &mx);
    if (mn > 0) continue;  // strict : mn == 0 descend (coquilles a voir)
    if (is_leaf(z)) {
      if (st) ++st->leaf_tests;
      const i32 u = leaf_index(z);
      const i128 pw = k.power(ix.upos[(size_t)u]);
      if (pw < 0 || (nonstrict && pw == 0)) {
        interior->push_back(u);
        if (interior->size() > interior_cap) return CensusStatus::kInteriorOverflow;
      } else if (pw == 0) {
        shell->push_back(u);
        if (shell->size() > shell_cap) return CensusStatus::kShellOverflow;
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return CensusStatus::kOk;
}

}  // namespace mhgp7

