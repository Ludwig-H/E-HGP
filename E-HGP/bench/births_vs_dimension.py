"""Taille de sortie de la tour : naissances topologiques contre dimension.

C'est la mesure de l'obstruction, prise directement SUR L'OBJET et non sur un
substitut. Deux quantites sont comptees pour chaque ordre `k` :

* `apparitions` : le nombre de sommets de `Gamma_k` qui apparaissent, c'est-a-dire
  `C(n, k)` en toute dimension. Ce n'est PAS la taille de la sortie.
* `naissances` : le nombre de composantes connexes de `pi_0` creees, c'est-a-dire
  les composantes dont aucun sommet n'etait actif au niveau precedent. C'est la
  taille de la sortie : le nombre de feuilles de l'arbre de fusion d'ordre `k`.

Confondre les deux est une faute : un sommet peut naitre deja relie a un
voisin par une coface de meme niveau. Fixture permanente qui separe les deux
notions : `A = (0, 0)`, `B = (2, 0)`, `C = (1, 1)` dans le plan, ordre 2. La
boule englobante minimale de `{A, B}` a pour centre `(1, 0)` et rayon carre
`1` ; `C` est a distance carre `1` de ce centre, donc SUR la sphere. La boule
FERMEE n'est donc pas vide, alors que l'interieur strict l'est : il y a deux
naissances topologiques et non trois. Le critere de naissance est la boule
fermee.

Usage :

    python3 bench/births_vs_dimension.py --n 8 --k-max 4 --dims 2,3,5,10,20,50 --seeds 5,17,31

Code de sortie : 0 conforme, 2 refus avant calcul, 3 plancher viole.
"""

import argparse
import random
import sys
from math import comb
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.exact.tower import FullTower

EXIT_OK = 0
EXIT_REFUSED = 2
EXIT_FLOOR = 3


def fixture_check():
    """Verifie la fixture permanente ferme / ouvert (retourne True si conforme)."""
    tower = FullTower([(0, 0), (2, 0), (1, 1)], 3)
    state = tower.states[2]
    return len(state.vertex_appearances) == 3 and len(state.component_births) == 2


def main(argv=None):
    parser = argparse.ArgumentParser(description="naissances topologiques contre dimension")
    parser.add_argument("--n", type=int, default=8)
    parser.add_argument("--k-max", type=int, default=4)
    parser.add_argument("--dims", type=str, default="2,3,5,10,20,50")
    parser.add_argument("--seeds", type=str, default="5,17,31")
    parser.add_argument("--extent", type=int, default=1000)
    options = parser.parse_args(argv)

    dimensions = [int(value) for value in options.dims.split(",") if value]
    seeds = [int(value) for value in options.seeds.split(",") if value]
    if not dimensions or not seeds or options.n < 3:
        return EXIT_REFUSED
    if not fixture_check():
        return EXIT_FLOOR

    orders = list(range(1, min(options.k_max, options.n) + 1))
    print(
        "n={} graines={} : naissances topologiques (taille de sortie) / C(n,k)".format(
            options.n, ",".join(str(seed) for seed in seeds)
        )
    )
    print(
        "{:>5} | ".format("d")
        + " | ".join("k={:<12}".format(order) for order in orders)
    )
    print(
        "{:>5} | ".format("")
        + " | ".join("{:<14}".format("/ " + str(comb(options.n, order))) for order in orders)
    )
    measured = 0
    for dimension in dimensions:
        rows = {order: [] for order in orders}
        for seed in seeds:
            generator = random.Random(seed * 100 + dimension)
            cloud = [
                tuple(generator.randint(0, options.extent) for _ in range(dimension))
                for _ in range(options.n)
            ]
            if len(set(cloud)) < len(cloud):
                continue
            tower = FullTower(cloud, options.k_max)
            for order in orders:
                state = tower.states.get(order)
                if state is None:
                    continue
                rows[order].append(len(state.component_births))
                measured += 1
        cells = []
        for order in orders:
            values = rows[order]
            if not values:
                cells.append("{:<14}".format("-"))
                continue
            mean = sum(values) / len(values)
            cells.append("{:<14}".format("{:.1f} ({}-{})".format(mean, min(values), max(values))))
        print("{:>5} | ".format(dimension) + " | ".join(cells))
    if measured < len(dimensions):
        return EXIT_FLOOR
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
