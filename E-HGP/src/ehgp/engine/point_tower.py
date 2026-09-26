"""Tour E-HGP au niveau des observations, exacte et libre en dimension.

La tour FULL de `exact/tower.py` vit sur les `C(n, k)` parties de cardinal
`k` : sa TAILLE elle-meme explose en grande dimension (cf.
`docs/OBSTRUCTION_GRANDE_DIMENSION.md`). Le moteur E-HGP calcule donc la
PROJECTION de cette tour sur les observations : pour chaque ordre `k`,
l'arbre de fusion des observations, ou

* l'observation `x_i` nait au niveau `a_k(x_i)` (sa k-ieme distance au carre,
  la premiere etant nulle) ;
* deux observations sont reliees au niveau `w_ij^{(k)}`, maximum de `a_k` sur
  le SEGMENT `[x_i, x_j]`, calcule exactement par `segment.py`.

Un segment contenu dans `L_k(a)` prouve que ses extremites sont dans la meme
composante : le niveau de fusion vrai est donc AU PLUS `w_ij^{(k)}`. La tour
des segments est ainsi un MAJORANT certifie de la projection exacte, jamais
une approximation non gardee. L'egalite avec la projection exacte est une
question mesuree, pas declaree (voir `bench/`).

Les applications verticales sont gratuites et exactes : `L_k(a)` est inclus
dans `L_{k-1}(a)`, donc la composante d'une observation a l'ordre `k`
s'envoie sur sa composante a l'ordre `k - 1`, a la meme observation.
"""

import hashlib
import json
from itertools import combinations

from .segment import entry_levels, rational_cloud, segment_maximum


class _Forest:
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
        left_root, right_root = self.find(left), self.find(right)
        if left_root == right_root:
            return False
        keep, drop = (
            (left_root, right_root) if left_root < right_root else (right_root, left_root)
        )
        self.parent[drop] = keep
        return True


class PointTower:
    """Arbre de fusion des observations pour tous les ordres `1..k_max`."""

    def __init__(self, cloud, k_max):
        self.cloud = rational_cloud(cloud)
        self.count = len(self.cloud)
        self.dimension = len(self.cloud[0])
        self.k_max = k_max
        self.effective = min(k_max, self.count)
        self.entry = entry_levels(self.cloud, self.effective)
        self.weights = {}
        self.witness_times = {}
        for source, target in combinations(range(self.count), 2):
            for order in range(1, self.effective + 1):
                level, time = segment_maximum(self.cloud, source, target, order)
                self.weights[(order, source, target)] = level
                self.witness_times[(order, source, target)] = time
        self.merges = {}
        self.ultrametric = {}
        for order in range(1, self.effective + 1):
            self._build_order(order)

    def _build_order(self, order):
        edges = sorted(
            (self.weights[(order, source, target)], source, target)
            for source, target in combinations(range(self.count), 2)
        )
        forest = _Forest(self.count)
        merges = []
        ultrametric = {}
        for level, source, target in edges:
            if forest.union(source, target):
                merges.append((level, source, target))
        forest = _Forest(self.count)
        for level, source, target in edges:
            root_source, root_target = forest.find(source), forest.find(target)
            if root_source == root_target:
                continue
            members_source = [
                index for index in range(self.count) if forest.find(index) == root_source
            ]
            members_target = [
                index for index in range(self.count) if forest.find(index) == root_target
            ]
            for left in members_source:
                for right in members_target:
                    key = (min(left, right), max(left, right))
                    if key not in ultrametric:
                        ultrametric[key] = level
            forest.union(source, target)
        self.merges[order] = merges
        self.ultrametric[order] = ultrametric

    def cophenetic(self, order):
        """Ultrametrique de liaison simple sur les poids de segment."""
        return self.ultrametric[order]

    def canonical_record(self):
        """Enregistrement canonique JSON-serialisable de la tour projetee."""
        orders = {}
        for order in range(1, self.effective + 1):
            births = [
                {"point": index, "level": [int(self.entry[index][order - 1].numerator),
                                           int(self.entry[index][order - 1].denominator)]}
                for index in range(self.count)
            ]
            merges = [
                {
                    "level": [int(level.numerator), int(level.denominator)],
                    "points": [source, target],
                }
                for level, source, target in self.merges[order]
            ]
            orders[str(order)] = {"births": births, "merges": merges}
        return {
            "object": "ehgp.point_tower.v1",
            "count": self.count,
            "dimension": self.dimension,
            "k_max": self.k_max,
            "k_effective": self.effective,
            "orders": orders,
        }

    def digest(self):
        """Digest canonique sha256 de la tour projetee."""
        payload = json.dumps(
            self.canonical_record(), sort_keys=True, separators=(",", ":"), ensure_ascii=True
        )
        return hashlib.sha256(payload.encode("ascii")).hexdigest()

    def vertical_arrows(self, order, level):
        """Applications verticales : composante a l'ordre `k` vers `k - 1`."""
        upper = self._components(order, level)
        lower = self._components(order - 1, level)
        arrows = []
        for representative, members in sorted(upper.items()):
            targets = {lower_key for lower_key, lower_members in lower.items()
                       if set(members) & set(lower_members)}
            arrows.append((representative, sorted(targets)))
        return arrows

    def _components(self, order, level):
        forest = _Forest(self.count)
        present = [
            index for index in range(self.count) if self.entry[index][order - 1] <= level
        ]
        for merge_level, source, target in self.merges[order]:
            if merge_level <= level:
                forest.union(source, target)
        groups = {}
        for index in present:
            groups.setdefault(forest.find(index), []).append(index)
        return groups
