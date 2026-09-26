"""Tour FULL exacte de l'objet HGP, libre en dimension.

L'objet calcule est celui de `docs/SPECIFICATION_MORSEHGP3D.md` § 3-4, ecrit
sans aucune hypothese sur la dimension ambiante :

* pour `y` dans `R^d`, `a_k(y)` est la k-ieme plus petite distance au carre
  aux observations et `L_k(a) = {y : a_k(y) <= a}` est la region couverte par
  au moins `k` boules fermees de rayon `sqrt(a)` centrees sur `X` ;
* `Gamma_k(a)` est le graphe dont les sommets sont les `F` inclus dans `X` de
  cardinal `k` tels que `beta(F) <= a`, deux sommets etant adjacents lorsque
  `|F union F'| = k + 1` et `beta(F union F') <= a` ;
* le theoreme 2 du manuscrit identifie `pi_0(L_k(a))` et `pi_0(Gamma_k(a))`.

La tour FULL est la donnee, pour `k = 1..K`, de l'arbre de fusion de
`Gamma_k` (naissances, multifusions groupees par niveau, niveaux exacts) ET
des applications verticales `pi_0(Gamma_k(a)) -> pi_0(Gamma_{k-1}(a))`.

Ce module est un ORACLE borne : il enumere `C(n, k)` sommets et
`C(n, k + 1)` generateurs d'aretes. Il etablit la verite pour `n <= 14`, il
n'est jamais un backend. Toute decision y est rationnelle exacte.
"""

import hashlib
import json
from itertools import combinations

from .meb import SupportCatalog


class UnionFind:
    """Union-find deterministe : la racine conservee est le plus petit indice."""

    def __init__(self, size):
        self.parent = list(range(size))

    def find(self, item):
        root = item
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[item] != root:
            self.parent[item], item = root, self.parent[item]
        return root

    def union(self, left, right):
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return left_root
        keep, drop = (left_root, right_root) if left_root < right_root else (right_root, left_root)
        self.parent[drop] = keep
        return keep


def _level_pair(level):
    return [int(level.numerator), int(level.denominator)]


def _mask_of(subset):
    mask = 0
    for index in subset:
        mask |= 1 << index
    return mask


def _points_of(mask):
    points = []
    index = 0
    while mask:
        if mask & 1:
            points.append(index)
        mask >>= 1
        index += 1
    return tuple(points)


class OrderState:
    """Etat courant de `Gamma_k` pendant le balayage des niveaux."""

    def __init__(self, catalog, order):
        self.order = order
        self.count = catalog.count
        self.subsets = list(combinations(range(self.count), order))
        self.index_of = {_mask_of(subset): index for index, subset in enumerate(self.subsets)}
        self.union_find = UnionFind(len(self.subsets))
        self.active = [False] * len(self.subsets)
        self.minimum = [None] * len(self.subsets)
        self.union_mask = [0] * len(self.subsets)
        self.oldest = [None] * len(self.subsets)
        self.components = 0
        self.vertex_appearances = []
        self.component_births = []
        self.merges = []
        self.events = {}
        for subset in self.subsets:
            mask = _mask_of(subset)
            level = catalog.beta_mask(mask)
            entry = self.events.setdefault(level, ([], []))
            entry[0].append(mask)
        if order + 1 <= self.count:
            for subset in combinations(range(self.count), order + 1):
                mask = _mask_of(subset)
                level = catalog.beta_mask(mask)
                entry = self.events.setdefault(level, ([], []))
                entry[1].append(mask)

    def apply_level(self, level):
        """Applique naissances puis fusions du niveau, renvoie True si change."""
        entry = self.events.get(level)
        if entry is None:
            return False
        born, generators = entry
        touched = []
        for mask in sorted(born):
            index = self.index_of[mask]
            self.active[index] = True
            self.minimum[index] = self.subsets[index]
            self.union_mask[index] = mask
            self.oldest[index] = level
            self.components += 1
            self.vertex_appearances.append((level, self.subsets[index]))
            touched.append(index)
        links = []
        for generator_mask in sorted(generators):
            roots = []
            remaining = generator_mask
            while remaining:
                bit = remaining & -remaining
                remaining ^= bit
                child_index = self.index_of.get(generator_mask ^ bit)
                if child_index is None or not self.active[child_index]:
                    continue
                root = self.union_find.find(child_index)
                if root not in roots:
                    roots.append(root)
            if len(roots) >= 2:
                links.append((tuple(sorted(roots)), _points_of(generator_mask)))
        if links:
            touched.extend(self._apply_links(level, links))
        self._record_component_births(level, touched)
        return bool(born) or bool(links)

    def _record_component_births(self, level, touched):
        """Naissances TOPOLOGIQUES : composantes dont tous les sommets naissent ici.

        Une apparition de sommet n'est pas une naissance de composante : un
        sommet peut naitre deja relie a un voisin par une coface de meme
        niveau. La naissance topologique est la creation d'une composante de
        `pi_0`, c'est-a-dire une composante dont AUCUN sommet n'etait actif
        avant ce niveau. Confondre les deux fait compter `C(n, k)` naissances
        en toute dimension, ce qui est faux et rend tout argument de taille
        de sortie sans valeur.
        """
        seen = set()
        for index in touched:
            if not self.active[index]:
                continue
            root = self.union_find.find(index)
            if root in seen:
                continue
            seen.add(root)
            if self.oldest[root] == level:
                self.component_births.append((level, self.minimum[root]))

    def _apply_links(self, level, links):
        """Groupe les liens simultanes en multifusions canoniques."""
        local_parent = {}

        def local_find(item):
            while local_parent[item] != item:
                item = local_parent[item]
            return item

        for roots, _witness in links:
            for root in roots:
                local_parent.setdefault(root, root)
            first = local_find(roots[0])
            for root in roots[1:]:
                other = local_find(root)
                if other != first:
                    if other < first:
                        local_parent[first] = other
                        first = other
                    else:
                        local_parent[other] = first
        survivors = []
        groups = {}
        for root in local_parent:
            groups.setdefault(local_find(root), []).append(root)
        witnesses_of = {}
        for roots, witness in links:
            witnesses_of.setdefault(local_find(roots[0]), []).append(witness)
        for group_root in sorted(groups):
            roots = sorted(groups[group_root])
            merged = [
                {
                    "representative": self.minimum[root],
                    "union": _points_of(self.union_mask[root]),
                }
                for root in roots
            ]
            keep = roots[0]
            merged_union = 0
            merged_minimum = None
            merged_oldest = None
            for root in roots:
                merged_union |= self.union_mask[root]
                candidate = self.minimum[root]
                if merged_minimum is None or candidate < merged_minimum:
                    merged_minimum = candidate
                age = self.oldest[root]
                if age is not None and (merged_oldest is None or age < merged_oldest):
                    merged_oldest = age
            for root in roots[1:]:
                keep = self.union_find.union(keep, root)
            keep = self.union_find.find(keep)
            self.union_mask[keep] = merged_union
            self.minimum[keep] = merged_minimum
            self.oldest[keep] = merged_oldest
            self.components -= len(roots) - 1
            survivors.append(keep)
            self.merges.append(
                (
                    level,
                    len(roots),
                    tuple(sorted(witnesses_of.get(group_root, []))),
                    tuple(sorted(item["representative"] for item in merged)),
                    tuple(sorted(item["union"] for item in merged)),
                    _points_of(merged_union),
                )
            )
        return survivors

    def partition(self):
        """Partition courante : tuple trie de (representant, union de points)."""
        seen = {}
        for index, active in enumerate(self.active):
            if not active:
                continue
            root = self.union_find.find(index)
            seen[root] = (self.minimum[root], _points_of(self.union_mask[root]))
        return tuple(sorted(seen.values()))

    def representative_of(self, subset):
        """Representant canonique de la composante d'un sommet actif."""
        index = self.index_of.get(_mask_of(subset))
        if index is None or not self.active[index]:
            return None
        return self.minimum[self.union_find.find(index)]


class FullTower:
    """Tour FULL exacte : tous les ordres et toutes les applications verticales."""

    def __init__(self, cloud, k_max, keep_history=False):
        if k_max < 1:
            raise ValueError("k_max doit valoir au moins 1")
        self.k_max = k_max
        self.catalog = SupportCatalog(cloud, k_max + 1)
        self.count = self.catalog.count
        self.dimension = self.catalog.dimension
        self.effective = min(k_max, self.count)
        self.states = {
            order: OrderState(self.catalog, order) for order in range(1, self.effective + 1)
        }
        levels = set()
        for state in self.states.values():
            levels.update(state.events)
        self.levels = sorted(levels)
        self.vertical = []
        self.history = [] if keep_history else None
        self._sweep()

    def _sweep(self):
        previous_arrows = None
        for level in self.levels:
            changed = False
            for order in range(1, self.effective + 1):
                if self.states[order].apply_level(level):
                    changed = True
            arrows = self._arrows()
            if changed or previous_arrows is None or arrows != previous_arrows:
                self.vertical.append((level, arrows))
                previous_arrows = arrows
            if self.history is not None:
                self.history.append(
                    (
                        level,
                        tuple(
                            self.states[order].components
                            for order in range(1, self.effective + 1)
                        ),
                    )
                )

    def _arrows(self):
        arrows = []
        for order in range(2, self.effective + 1):
            upper = self.states[order]
            lower = self.states[order - 1]
            per_order = []
            for representative, _union in upper.partition():
                child = representative[1:]
                target = lower.representative_of(child)
                per_order.append((representative, target))
            arrows.append((order, tuple(per_order)))
        return tuple(arrows)

    def component_counts(self):
        """Nombre courant de composantes par ordre, au dernier niveau balaye."""
        return {order: self.states[order].components for order in self.states}

    def partition_at(self, order, level):
        """Partition de `pi_0(Gamma_order(level))`, rejouee depuis le debut."""
        replay = OrderState(self.catalog, order)
        for candidate in self.levels:
            if candidate > level:
                break
            replay.apply_level(candidate)
        return replay.partition()

    def canonical_record(self):
        """Enregistrement canonique JSON-serialisable de la tour complete."""
        orders = {}
        for order, state in self.states.items():
            appearances = [
                {"level": _level_pair(level), "vertex": list(subset)}
                for level, subset in sorted(state.vertex_appearances)
            ]
            births = [
                {"level": _level_pair(level), "component": list(subset)}
                for level, subset in sorted(state.component_births)
            ]
            merges = []
            for level, arity, witnesses, representatives, unions, result in sorted(state.merges):
                merges.append(
                    {
                        "level": _level_pair(level),
                        "arity": arity,
                        "witnesses": [list(witness) for witness in witnesses],
                        "merged_representatives": [list(item) for item in representatives],
                        "merged_unions": [list(item) for item in unions],
                        "union": list(result),
                    }
                )
            orders[str(order)] = {
                "vertex_appearances": appearances,
                "component_births": births,
                "merges": merges,
            }
        vertical = []
        for level, arrows in self.vertical:
            vertical.append(
                {
                    "level": _level_pair(level),
                    "arrows": [
                        {
                            "order": order,
                            "map": [
                                [list(source), None if target is None else list(target)]
                                for source, target in per_order
                            ],
                        }
                        for order, per_order in arrows
                    ],
                }
            )
        return {
            "object": "ehgp.full_tower.v1",
            "count": self.count,
            "dimension": self.dimension,
            "k_max": self.k_max,
            "k_effective": self.effective,
            "levels": [_level_pair(level) for level in self.levels],
            "orders": orders,
            "vertical": vertical,
        }

    def digest(self):
        """Digest canonique sha256 de la tour FULL."""
        payload = json.dumps(
            self.canonical_record(), sort_keys=True, separators=(",", ":"), ensure_ascii=True
        )
        return hashlib.sha256(payload.encode("ascii")).hexdigest()

    def merge_levels(self, order):
        """Multiensemble trie des niveaux de fusion de l'ordre donne."""
        levels = []
        for record in self.states[order].merges:
            level, arity = record[0], record[1]
            levels.extend([level] * (arity - 1))
        return sorted(levels)
