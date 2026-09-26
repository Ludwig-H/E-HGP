"""Tour projetee avec temoins : resserrer le majorant certifie.

`point_tower.py` ne relie que des paires d'OBSERVATIONS par un segment
droit. Ce n'est pas assez : un chemin de `L_k(a)` peut passer loin de toute
observation. Exemple gravé (carré de côté 10, ordre 3) : le niveau de fusion
exact de deux sommets adjacents vaut 100, alors que le segment qui les joint
exige 125 ; le chemin qui réalise 100 passe par le centre du carré, qui
n'est pas une observation.

Le centre du carré est precisement une SPHERE CRITIQUE : c'est un point fixe
de la descente MEB-Lloyd de `critical.py`. D'ou l'architecture :

1. produire des TEMOINS par descente (les centres critiques de masse `k` et
   `k + 1`, obtenus sans aucune enumeration combinatoire) ;
2. relier tous les temoins et toutes les observations par des segments, dont
   le niveau est certifie EXACTEMENT par `segment.py` ;
3. liaison simple sur ce graphe, puis restriction aux observations.

Le resultat reste un MAJORANT certifie du niveau de fusion exact : un chemin
polygonal contenu dans `L_k(a)` prouve la connexite, jamais l'inverse.
Ajouter des temoins ne peut que faire BAISSER le majorant. L'ecart residuel
au niveau exact est mesure contre l'oracle, jamais declare nul.
"""

from itertools import combinations

from .critical import critical_catalogue
from .segment import rational_cloud, segment_maximum_points


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


def witness_points(cloud, order, triples=False):
    """Temoins : centres critiques de masse `order` et `order + 1`."""
    points = rational_cloud(cloud)
    count = len(points)
    witnesses = []
    statistics = {}
    for mass in (order, order + 1):
        if mass > count:
            continue
        catalogue, stats = critical_catalogue(points, mass, triples=triples)
        statistics[mass] = stats
        for center, _level in catalogue:
            witnesses.append(center)
    unique = []
    seen = set()
    for center in witnesses:
        if center not in seen:
            seen.add(center)
            unique.append(center)
    return unique, statistics


def witness_ultrametric(cloud, order, triples=False):
    """Ultrametrique certifiee sur les observations, temoins inclus.

    Renvoie `(ultrametrique, statistiques)`. L'ultrametrique est indexee par
    les paires `(i, j)` d'observations, `i < j`, et donne un MAJORANT du
    niveau de fusion exact a l'ordre `order`.
    """
    points = rational_cloud(cloud)
    count = len(points)
    witnesses, statistics = witness_points(points, order, triples=triples)
    nodes = list(points) + [center for center in witnesses if center not in set(points)]
    total = len(nodes)
    edges = []
    for left, right in combinations(range(total), 2):
        level, _time = segment_maximum_points(points, nodes[left], nodes[right], order)
        edges.append((level, left, right))
    edges.sort()
    forest = _Forest(total)
    ultrametric = {}
    for level, left, right in edges:
        left_root, right_root = forest.find(left), forest.find(right)
        if left_root == right_root:
            continue
        members_left = [index for index in range(total) if forest.find(index) == left_root]
        members_right = [index for index in range(total) if forest.find(index) == right_root]
        for first in members_left:
            if first >= count:
                continue
            for second in members_right:
                if second >= count:
                    continue
                key = (min(first, second), max(first, second))
                if key not in ultrametric:
                    ultrametric[key] = level
        forest.union(left, right)
    statistics["nodes"] = total
    statistics["witnesses"] = total - count
    statistics["edges"] = len(edges)
    return ultrametric, statistics
